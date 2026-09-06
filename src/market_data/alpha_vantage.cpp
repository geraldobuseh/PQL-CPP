#include "market_data/alpha_vantage.hpp"
#include "market_data/validation.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include <thread>
#include <nlohmann/json.hpp>

namespace pql {
namespace {
[[noreturn]] void malformed() { throw MarketDataValidationError(MarketDataIssue::MalformedResponse); }
void supported(const Symbol& symbol) {
    const auto& symbols = supportedMarketSymbols();
    if (std::find(symbols.begin(),symbols.end(),symbol)==symbols.end())
        throw MarketDataError("Unsupported market-data symbol");
}
std::string normalized(std::string value) {
    if (value.empty() || value.front()=='.' || value.back()=='.') malformed();
    bool dot=false;
    for (char c : value) {
        if (c=='.' && !dot) dot=true;
        else if (c<'0' || c>'9') malformed();
    }
    const auto first=value.find_first_not_of('0');
    value=first==std::string::npos ? "0" : value.substr(first);
    if (value.front()=='.') value="0"+value;
    if (value.find('.')!=std::string::npos) {
        while(value.back()=='0') value.pop_back();
        if(value.back()=='.') value.pop_back();
    }
    return value;
}
double number(const nlohmann::json& field) {
    if (!field.is_string()) malformed();
    const auto text=field.get<std::string>();
    const auto canonical=normalized(text);
    double value{};
    const auto result=std::from_chars(text.data(),text.data()+text.size(),value);
    if (result.ec!=std::errc{} || result.ptr!=text.data()+text.size() || !std::isfinite(value)) malformed();
    std::array<char,1024> buffer{};
    // Match persistence's GENERAL shortest-round-trip protocol exactly. Fixed
    // formatting can describe a different exact decimal for a large double.
    const auto out=std::to_chars(buffer.data(),buffer.data()+buffer.size(),value);
    if (out.ec!=std::errc{}) malformed();
    std::string serialized(buffer.data(),out.ptr);
    const auto exponent=serialized.find('e');
    if(exponent!=std::string::npos) {
        const int shift=std::stoi(serialized.substr(exponent+1));
        auto digits=serialized.substr(0,exponent);
        const auto dot=digits.find('.');
        int point=static_cast<int>(dot==std::string::npos?digits.size():dot)+shift;
        if(dot!=std::string::npos) digits.erase(dot,1);
        if(point<=0) serialized="0."+std::string(static_cast<std::size_t>(-point),'0')+digits;
        else if(static_cast<std::size_t>(point)>=digits.size())
            serialized=digits+std::string(static_cast<std::size_t>(point)-digits.size(),'0');
        else { digits.insert(static_cast<std::size_t>(point),1,'.'); serialized=digits; }
    }
    if(normalized(serialized)!=canonical) malformed();
    return value;
}

Price readPrice(const nlohmann::json& row,const char* key,MarketDataField field,Date date) {
    if(!row.contains(key) || row.at(key).is_null() ||
       (row.at(key).is_string() && row.at(key).get<std::string>().find_first_not_of(" \t\r\n")==std::string::npos))
        throw MarketDataValidationError(MarketDataIssue::MissingPrice,field,date);
    std::optional<Price> price;
    try { price=Price::create(number(row.at(key))); }
    catch(const MarketDataValidationError&) {
        throw MarketDataValidationError(MarketDataIssue::InvalidPrice,field,date);
    }
    if(!price) throw MarketDataValidationError(MarketDataIssue::InvalidPrice,field,date);
    return *price;
}

Quantity readVolume(const nlohmann::json& row,Date date) {
    std::optional<Quantity> volume;
    try {
        if(row.contains("5. volume")) volume=Quantity::create(number(row.at("5. volume")));
    } catch(const MarketDataValidationError&) {
        throw MarketDataValidationError(MarketDataIssue::InvalidVolume,MarketDataField::Volume,date);
    }
    if(!volume || std::floor(volume->value())!=volume->value())
        throw MarketDataValidationError(MarketDataIssue::InvalidVolume,MarketDataField::Volume,date);
    return *volume;
}
}

const std::vector<Symbol>& supportedMarketSymbols() {
    static const std::vector<Symbol> symbols{*Symbol::create("SPY"),*Symbol::create("QQQ"),
        *Symbol::create("AAPL"),*Symbol::create("MSFT"),*Symbol::create("NVDA"),
        *Symbol::create("AMZN"),*Symbol::create("GOOGL"),*Symbol::create("META")};
    return symbols;
}
Date parseMarketDate(const std::string& text) {
    const auto invalid=[] { throw MarketDataValidationError(MarketDataIssue::InvalidDate,MarketDataField::SessionDate); };
    if (text.size()!=10 || text[4]!='-' || text[7]!='-') invalid();
    for (std::size_t i=0;i<text.size();++i)
        if (i!=4 && i!=7 && (text[i]<'0' || text[i]>'9')) invalid();
    const auto date=Date::create(std::stoi(text.substr(0,4)),
        static_cast<unsigned>(std::stoi(text.substr(5,2))),static_cast<unsigned>(std::stoi(text.substr(8,2))));
    if (!date) invalid();
    return *date;
}
std::string formatMarketDate(Date date) {
    const auto value=date.value();
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setfill('0') << std::setw(4) << int(value.year()) << '-'
        << std::setw(2) << unsigned(value.month()) << '-' << std::setw(2) << unsigned(value.day());
    return out.str();
}

std::vector<PriceBar> parseAlphaVantageDaily(const std::string& response,const Symbol& symbol,Date today) {
    supported(symbol);
    if (response.size()>2*1024*1024) malformed();
    try {
        struct ObjectKeys { std::set<std::string> names; bool daily; };
        std::vector<ObjectKeys> keys;
        std::string pending_key;
        const auto data=nlohmann::json::parse(response,[&](int depth,nlohmann::json::parse_event_t event,nlohmann::json& value) {
            if (depth>16) malformed();
            if(event==nlohmann::json::parse_event_t::object_start) {
                keys.push_back({{},depth==1 && pending_key=="Time Series (Daily)"});
                pending_key.clear();
            }
            if(event==nlohmann::json::parse_event_t::key) {
                pending_key=value.get<std::string>();
                if(!keys.back().names.insert(pending_key).second) {
                    if(keys.back().daily)
                        throw MarketDataValidationError(MarketDataIssue::DuplicateDate,MarketDataField::SessionDate,parseMarketDate(pending_key));
                    malformed();
                }
            }
            if(event==nlohmann::json::parse_event_t::object_end) keys.pop_back();
            return true;
        });
        if(data.contains("Note") || data.contains("Information"))
            throw MarketDataError("Alpha Vantage quota or entitlement response; retry after checking provider limits");
        if(data.contains("Error Message")) throw MarketDataError("Alpha Vantage rejected the request");
        const auto& meta=data.at("Meta Data");
        if(meta.at("2. Symbol")!=symbol.value() || meta.at("5. Time Zone")!="US/Eastern") malformed();
        const auto refreshed=[&] {
            try { return parseMarketDate(meta.at("3. Last Refreshed").get<std::string>()); }
            catch(const MarketDataValidationError&) {
                throw MarketDataValidationError(MarketDataIssue::InvalidDate,MarketDataField::LastRefreshed);
            }
        }();
        if(refreshed>today) throw MarketDataValidationError(MarketDataIssue::FutureTimestamp,MarketDataField::LastRefreshed,refreshed);
        const auto& series=data.at("Time Series (Daily)");
        if(!series.is_object() || series.empty() || series.size()>100) malformed();
        std::vector<PriceBar> bars;
        std::optional<Date> newest;
        // JSON object keys are unordered semantically; nlohmann's object stores
        // ISO date keys in ascending order. Duplicate keys were rejected above.
        for(const auto& [key,row] : series.items()) {
            const auto date=parseMarketDate(key);
            if(date>today) throw MarketDataValidationError(MarketDataIssue::FutureTimestamp,MarketDataField::SessionDate,date);
            newest=date;
            const auto open=readPrice(row,"1. open",MarketDataField::Open,date);
            const auto high=readPrice(row,"2. high",MarketDataField::High,date);
            const auto low=readPrice(row,"3. low",MarketDataField::Low,date);
            const auto close=readPrice(row,"4. close",MarketDataField::Close,date);
            const auto volume=readVolume(row,date);
            const auto bar=PriceBar::create(symbol,date,open,high,low,close,volume);
            if(!bar) {
                const auto field=high.value()<low.value()?MarketDataField::High:
                    (open.value()<low.value() || open.value()>high.value()?MarketDataField::Open:MarketDataField::Close);
                throw MarketDataValidationError(MarketDataIssue::InvalidOhlc,field,date);
            }
            if(date<today) bars.push_back(*bar); // Never ingest a potentially open current session.
        }
        if(!newest || *newest!=refreshed || bars.empty()) malformed();
        return bars;
    } catch(const nlohmann::json::exception&) { malformed(); }
}

AlphaVantageProvider::AlphaVantageProvider(HttpTransport& transport,std::string key,Date today,
                                         RetryObserver observer,RetryWait wait)
    : transport_(transport),api_key_(std::move(key)),today_(today),observer_(std::move(observer)),wait_(std::move(wait)) {
    // Alpha Vantage keys are alphanumeric; reject delimiters/NUL before URL binding.
    if(api_key_.empty() || !std::all_of(api_key_.begin(),api_key_.end(),[](unsigned char c) {
        return (c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9');
    })) throw MarketDataError("Configure a valid ALPHAVANTAGE_API_KEY");
    if(!wait_) wait_=[](std::chrono::seconds delay) { std::this_thread::sleep_for(delay); };
}
std::vector<PriceBar> AlphaVantageProvider::fetch(const Symbol& symbol) {
    supported(symbol);
    // Free accounts can throttle bursts even before the daily quota is reached.
    // Pace separate queries as well as retry attempts; tests inject a no-op wait.
    if(requested_) wait_(std::chrono::seconds{2});
    requested_=true;
    const auto url="https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&outputsize=compact&datatype=json&symbol="
        +symbol.value()+"&apikey="+api_key_;
    for(unsigned attempt=1;attempt<=3;++attempt) {
        if(observer_) observer_({symbol,attempt,false});
        std::chrono::seconds delay{1U<<(attempt-1)};
        try {
            const auto response=transport_.get(url);
            if(response.status==429 || response.status==408 || response.status>=500) {
                if(response.retry_after_seconds) {
                    if(*response.retry_after_seconds>30) throw MarketDataError("Provider retry delay exceeds this run's limit; rerun later");
                    delay=std::max(delay,std::chrono::seconds{*response.retry_after_seconds});
                }
                throw TransientMarketDataError("Transient provider HTTP failure");
            }
            if(response.status!=200) throw MarketDataError("Provider HTTP request rejected");
            return parseAlphaVantageDaily(response.body,symbol,today_);
        } catch(const TransientMarketDataError&) {
            if(attempt==3) throw;
            if(observer_) observer_({symbol,attempt,true});
            wait_(delay);
        }
    }
    throw MarketDataError("Provider attempts exhausted");
}
Price AlphaVantageProvider::getLatestPrice(const Symbol& symbol) { return fetch(symbol).back().close(); }
std::vector<PriceBar> AlphaVantageProvider::getHistory(const Symbol& symbol,Date from,Date to) {
    if(from>to) throw std::invalid_argument("Reversed history range");
    if(to>=today_) throw MarketDataError("History must end before current UTC date");
    const auto bars=fetch(symbol);
    if(from<bars.front().date() || to>bars.back().date())
        throw MarketDataError("Requested range exceeds returned compact completed-day coverage");
    std::vector<PriceBar> result;
    for(const auto& bar:bars) if(bar.date()>=from && bar.date()<=to) result.push_back(bar);
    return result;
}
} // namespace pql
