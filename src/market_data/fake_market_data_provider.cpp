#include "market_data/fake_market_data_provider.hpp"

namespace pql {

FakeMarketDataProvider::FakeMarketDataProvider(const std::vector<PriceBar>& bars) {
    for (const auto& bar : bars) {
        auto& history = bars_[bar.symbol().value()];
        if (!history.empty() && bar.date() <= history.back().date()) {
            throw MarketDataError("Fake history requires strictly ascending dates per symbol");
        }
        history.push_back(bar);
    }
}

const std::vector<PriceBar>& FakeMarketDataProvider::series(const Symbol& symbol) const {
    const auto found = bars_.find(symbol.value());
    if (found == bars_.end()) throw MarketDataError("Unknown fake market-data symbol");
    return found->second;
}

Price FakeMarketDataProvider::getLatestPrice(const Symbol& symbol) {
    return series(symbol).back().close();
}

std::vector<PriceBar> FakeMarketDataProvider::getHistory(const Symbol& symbol, Date from, Date to) {
    if (from > to) throw std::invalid_argument("Reversed history range");
    std::vector<PriceBar> result;
    for (const auto& bar : series(symbol)) {
        if (bar.date() > to) break;
        if (bar.date() >= from) result.push_back(bar);
    }
    return result;
}

}  // namespace pql
