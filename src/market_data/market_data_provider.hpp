#pragma once

#include <stdexcept>
#include <vector>

#include "domain/market_data.hpp"

namespace pql {

// Adapters translate unavailable/unknown-symbol, transport and malformed-data
// failures into this error. Do not expose credentials in diagnostic messages.
class MarketDataError : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

// Engine-facing seam: no HTTP, database, provider SDK or concrete adapter types.
// Callers borrow a provider reference; owning composition may use unique_ptr.
class MarketDataProvider {
   public:
    // Latest available raw USD-equity price; freshness is adapter policy. Throws
    // MarketDataError if unavailable. Not a point-in-time API for backtests.
    [[nodiscard]] virtual Price getLatestPrice(const Symbol& symbol) = 0;

    // Inclusive session dates [from,to]; from>to throws std::invalid_argument.
    // Completed raw daily bars only, matching symbol, strictly ascending unique
    // dates inside the range. Known nontrading dates need no synthetic bars.
    // Empty means a successful query with no observations; unknown symbols,
    // incomplete retrieval and malformed data throw MarketDataError instead.
    // Adapters enforce this collection contract; the abstract API cannot do so.
    [[nodiscard]] virtual std::vector<PriceBar> getHistory(const Symbol& symbol, Date from, Date to) = 0;

    virtual ~MarketDataProvider() = default;
};

}  // namespace pql
