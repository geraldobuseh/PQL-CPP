#pragma once

#include <optional>
#include <vector>

#include "domain/position.hpp"
#include "domain/transaction.hpp"

namespace pql {

struct MarketPrice {
    Symbol symbol;
    Price price;
};

// Detached strategy input. Owns its data and has no reference to a Portfolio or
// trade-application API. Future strategies consume this type and propose Orders.
class PortfolioSnapshot {
   public:
    // Copy from rvalues too: moved-from cash/history must not become inconsistent.
    PortfolioSnapshot(const PortfolioSnapshot&) = default;
    PortfolioSnapshot& operator=(PortfolioSnapshot other) noexcept;

    [[nodiscard]] PortfolioId id() const noexcept { return id_; }
    [[nodiscard]] Money startingCash() const noexcept { return starting_cash_; }
    [[nodiscard]] Money cashBalance() const noexcept { return cash_; }
    [[nodiscard]] const std::vector<Position>& positions() const noexcept { return positions_; }
    [[nodiscard]] const std::vector<Transaction>& transactionHistory() const noexcept {
        return history_;
    }
    [[nodiscard]] std::optional<Money> marketValue(const std::vector<MarketPrice>& marks) const;
    [[nodiscard]] std::optional<Money> totalValue(const std::vector<MarketPrice>& marks) const;
    bool operator==(const PortfolioSnapshot&) const = default;

   private:
    friend class Portfolio;
    PortfolioSnapshot(PortfolioId id, Money starting_cash);
    void swap(PortfolioSnapshot& other) noexcept;

    PortfolioId id_;
    Money starting_cash_;
    Money cash_;
    std::vector<Position> positions_;
    std::vector<Transaction> history_;
};

// Owned by trusted execution/orchestration. Never pass this authority to a strategy;
// pass snapshot() instead. Only accepted Transaction records change financial state.
class Portfolio {
   public:
    [[nodiscard]] static std::optional<Portfolio> create(PortfolioId id, Money starting_cash);
    [[nodiscard]] static std::optional<Portfolio> replay(PortfolioId id, Money starting_cash,
                                                         const std::vector<Transaction>& history);

    // False rejects without changing cash, positions or history. Allocation failures
    // may throw, also without changing state. OrderId is unique within this portfolio.
    [[nodiscard]] bool applyTransaction(const Transaction& transaction);
    // Compatibility adapter: an explicitly zero-fee fill, attributed to this portfolio.
    [[nodiscard]] bool applyTrade(const Trade& trade);

    [[nodiscard]] PortfolioSnapshot snapshot() const { return state_; }
    [[nodiscard]] PortfolioId id() const noexcept { return state_.id(); }
    [[nodiscard]] Money startingCash() const noexcept { return state_.startingCash(); }
    [[nodiscard]] Money cashBalance() const noexcept { return state_.cashBalance(); }
    [[nodiscard]] const std::vector<Position>& positions() const noexcept {
        return state_.positions();
    }
    [[nodiscard]] const std::vector<Transaction>& transactionHistory() const noexcept {
        return state_.transactionHistory();
    }
    [[nodiscard]] std::optional<Money> marketValue(const std::vector<MarketPrice>& marks) const {
        return state_.marketValue(marks);
    }
    [[nodiscard]] std::optional<Money> totalValue(const std::vector<MarketPrice>& marks) const {
        return state_.totalValue(marks);
    }

   private:
    Portfolio(PortfolioId id, Money starting_cash) : state_(id, starting_cash) {}
    PortfolioSnapshot state_;
};

}  // namespace pql
