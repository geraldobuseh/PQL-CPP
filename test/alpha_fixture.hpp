#pragma once
#include <nlohmann/json.hpp>
#include "market_data/alpha_vantage.hpp"

inline nlohmann::json alphaFixture(const std::string& symbol="AAPL") {
    return {{"Meta Data",{{"2. Symbol",symbol},{"3. Last Refreshed","2026-01-06"},{"5. Time Zone","US/Eastern"}}},
        {"Time Series (Daily)",{
            {"2026-01-02",{{"1. open","100.0000"},{"2. high","110.0000"},{"3. low","90.0000"},{"4. close","105.0000"},{"5. volume","1000"}}},
            {"2026-01-05",{{"1. open","105.0000"},{"2. high","120.0000"},{"3. low","100.0000"},{"4. close","115.0000"},{"5. volume","2000"}}},
            {"2026-01-06",{{"1. open","115.0000"},{"2. high","120.0000"},{"3. low","110.0000"},{"4. close","119.0000"},{"5. volume","500"}}}
        }}};
}
class FixtureTransport final : public pql::HttpTransport {
 public:
    std::vector<pql::HttpResponse> responses;
    std::size_t calls{0};
    bool timeout_first{false};
    pql::HttpResponse get(const std::string& url) override {
        if(!url.starts_with("https://www.alphavantage.co/query?")) throw pql::MarketDataError("Unexpected test URL");
        const auto index=calls++;
        if(timeout_first && index==0) throw pql::TransientMarketDataError("Synthetic timeout");
        return responses.at(std::min(index,responses.size()-1));
    }
};
