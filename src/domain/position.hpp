#pragma once

#include <optional>

#include "domain/trade.hpp"

namespace pql {

// Long-only weighted-average research accounting in one currency, before fees.
// Derived from caller-supplied unique fills; not a durable ledger or cash account.
class Position {
   public:
    [[nodiscard]] static Position empty(const Symbol& symbol);

    // Returns new state; rejection never changes this position. Fill times must be
    // nondecreasing. Equal timestamps preserve the caller's replay order.
    [[nodiscard]] std::optional<Position> with_trade(const Trade& trade) const;

    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] Quantity quantity() const noexcept { return quantity_; }
    [[nodiscard]] std::optional<Price> average_cost() const noexcept { return average_cost_; }
    [[nodiscard]] Money realized_pnl() const noexcept { return realized_pnl_; }

    // A flat position has zero basis, market value and unrealized P&L, and no average cost.
    // Realized P&L remains available after liquidation.
    // Arithmetic that cannot be represented returns nullopt, never a repaired value.
    [[nodiscard]] std::optional<Money> cost_basis() const;
    [[nodiscard]] std::optional<Money> market_value(Price mark) const;
    [[nodiscard]] std::optional<Money> unrealized_pnl(Price mark) const;
    bool operator==(const Position&) const = default;

   private:
    Position(const Symbol& symbol, Quantity quantity, std::optional<Price> average_cost,
             Money realized_pnl, std::optional<Timestamp> last_trade);

    Symbol symbol_;
    Quantity quantity_;
    std::optional<Price> average_cost_;
    Money realized_pnl_;
    std::optional<Timestamp> last_trade_;
};

}  // namespace pql
