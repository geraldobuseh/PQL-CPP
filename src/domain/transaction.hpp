#pragma once

#include <optional>

#include "domain/trade.hpp"

namespace pql {

// Immutable full-fill financial event. Identity is (portfolio_id, order_id).
// Copy construction preserves the event; assignment and field mutation are unavailable.
class Transaction {
   public:
    [[nodiscard]] static std::optional<Transaction> create(PortfolioId portfolio,
                                                           const Trade& trade, Money fees);
    Transaction(const Transaction&) = default;
    Transaction& operator=(const Transaction&) = delete;

    [[nodiscard]] PortfolioId portfolio_id() const noexcept { return portfolio_; }
    [[nodiscard]] OrderId order_id() const noexcept { return trade_.order_id(); }
    [[nodiscard]] const Symbol& symbol() const noexcept { return trade_.symbol(); }
    [[nodiscard]] OrderSide side() const noexcept { return trade_.side(); }
    [[nodiscard]] Quantity quantity() const noexcept { return trade_.quantity(); }
    [[nodiscard]] Price price() const noexcept { return trade_.price(); }
    [[nodiscard]] Money fees() const noexcept { return fees_; }
    [[nodiscard]] Timestamp timestamp() const noexcept { return trade_.timestamp(); }
    [[nodiscard]] const Trade& trade() const noexcept { return trade_; }
    bool operator==(const Transaction&) const = default;

   private:
    Transaction(PortfolioId portfolio, const Trade& trade, Money fees)
        : portfolio_(portfolio), trade_(trade), fees_(fees) {}

    PortfolioId portfolio_;
    Trade trade_;
    Money fees_;
};

}  // namespace pql
