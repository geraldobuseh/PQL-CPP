#pragma once

#include <map>
#include <string>

#include "market_data/market_data_provider.hpp"

namespace pql {

// Owns caller-supplied completed bars. No I/O, clock, randomness or fallback.
// Dates must be strictly ascending per symbol; interleaved symbols are allowed.
// Invalid fixture ordering/duplicates throw MarketDataError, never sort or repair.
class FakeMarketDataProvider final : public MarketDataProvider {
   public:
    explicit FakeMarketDataProvider(const std::vector<PriceBar>& bars);
    // Copy construction preserves both fixtures, including from rvalues. Avoid
    // partial container assignment compromising the validated ordering invariant.
    FakeMarketDataProvider(const FakeMarketDataProvider&) = default;
    FakeMarketDataProvider& operator=(const FakeMarketDataProvider&) = delete;

    // Last supplied close for the symbol, independent of history queries. This
    // does not advance a simulation clock or hide future fixture observations.
    [[nodiscard]] Price getLatestPrice(const Symbol& symbol) override;
    [[nodiscard]] std::vector<PriceBar> getHistory(const Symbol& symbol, Date from, Date to) override;

   private:
    [[nodiscard]] const std::vector<PriceBar>& series(const Symbol& symbol) const;
    std::map<std::string, std::vector<PriceBar>> bars_;
};

}  // namespace pql
