#include "domain/portfolio.hpp"

#include <algorithm>
#include <utility>

namespace pql {
namespace {

// Inputs are nonnegative amounts. Reject overflow or a positive term wholly lost
// to rounding, rather than reporting a value that omits cash or a holding.
std::optional<Money> add_amounts(Money left, Money right) {
    const double total = left.value() + right.value();
    if ((right.value() > 0.0 && total <= left.value()) ||
        (left.value() > 0.0 && total <= right.value())) {
        return std::nullopt;
    }
    return Money::create(total);
}

}  // namespace

PortfolioSnapshot::PortfolioSnapshot(PortfolioId id, Money starting_cash)
    : id_(id), starting_cash_(starting_cash), cash_(starting_cash) {}

PortfolioSnapshot& PortfolioSnapshot::operator=(PortfolioSnapshot other) noexcept {
    swap(other);
    return *this;
}

void PortfolioSnapshot::swap(PortfolioSnapshot& other) noexcept {
    using std::swap;
    swap(id_, other.id_);
    swap(starting_cash_, other.starting_cash_);
    swap(cash_, other.cash_);
    positions_.swap(other.positions_);
    history_.swap(other.history_);
}

std::optional<Money> PortfolioSnapshot::marketValue(const std::vector<MarketPrice>& marks) const {
    for (auto first = marks.begin(); first != marks.end(); ++first) {
        if (std::any_of(first + 1, marks.end(),
                        [&](const MarketPrice& other) { return first->symbol == other.symbol; })) {
            return std::nullopt;
        }
    }

    auto total = Money::create(0.0);
    // First-appearance position order is stable under deterministic trade replay.
    for (const auto& position : positions_) {
        if (position.quantity().value() == 0.0) {
            continue;
        }
        const auto mark = std::find_if(marks.begin(), marks.end(), [&](const MarketPrice& quote) {
            return quote.symbol == position.symbol();
        });
        if (mark == marks.end()) {
            return std::nullopt;
        }
        const auto value = position.market_value(mark->price);
        if (!value) {
            return std::nullopt;
        }
        total = add_amounts(*total, *value);
        if (!total) {
            return std::nullopt;
        }
    }
    return total;
}

std::optional<Money> PortfolioSnapshot::totalValue(const std::vector<MarketPrice>& marks) const {
    const auto market = marketValue(marks);
    if (!market) {
        return std::nullopt;
    }
    return add_amounts(cash_, *market);
}

std::optional<Portfolio> Portfolio::create(PortfolioId id, Money starting_cash) {
    if (starting_cash.value() < 0.0) {
        return std::nullopt;
    }
    return Portfolio{id, starting_cash};
}

std::optional<Portfolio> Portfolio::replay(PortfolioId id, Money starting_cash,
                                           const std::vector<Transaction>& history) {
    auto portfolio = create(id, starting_cash);
    if (!portfolio) {
        return std::nullopt;
    }
    for (const auto& transaction : history) {
        if (!portfolio->applyTransaction(transaction)) {
            return std::nullopt;
        }
    }
    return portfolio;
}

bool Portfolio::applyTrade(const Trade& trade) {
    const auto transaction = Transaction::create(id(), trade, *Money::create(0.0));
    return transaction && applyTransaction(*transaction);
}

bool Portfolio::applyTransaction(const Transaction& transaction) {
    if (transaction.portfolio_id() != id()) {
        return false;
    }
    const auto& trade = transaction.trade();
    if ((!state_.history_.empty() && trade.timestamp() < state_.history_.back().timestamp()) ||
        std::any_of(
            state_.history_.begin(), state_.history_.end(),
            [&](const Transaction& previous) { return previous.order_id() == trade.order_id(); })) {
        return false;
    }

    const auto notional = Money::create(trade.quantity().value() * trade.price().value());
    if (!notional || notional->value() <= 0.0) {
        return false;
    }
    // A sale can have zero or negative net proceeds when fees consume its value.
    std::optional<Money> cash_change;
    if (trade.side() == OrderSide::Buy) {
        const auto cost = add_amounts(*notional, transaction.fees());
        if (!cost) {
            return false;
        }
        cash_change = Money::create(-cost->value());
    } else {
        const double net = notional->value() - transaction.fees().value();
        if ((transaction.fees().value() > 0.0 && net == notional->value()) ||
            net == -transaction.fees().value()) {
            return false;
        }
        cash_change = Money::create(net);
    }
    if (!cash_change) {
        return false;
    }
    auto next_cash = std::optional<Money>{};
    if (cash_change->value() < 0.0) {
        if (-cash_change->value() > state_.cash_.value()) {
            return false;
        }
        next_cash = Money::create(state_.cash_.value() + cash_change->value());
        if (!next_cash || next_cash->value() < 0.0 || next_cash == state_.cash_) {
            return false;
        }
    } else {
        next_cash = add_amounts(state_.cash_, *cash_change);
        if (!next_cash) {
            return false;
        }
    }

    const auto current =
        std::find_if(state_.positions_.begin(), state_.positions_.end(),
                     [&](const Position& position) { return position.symbol() == trade.symbol(); });
    const auto next_position =
        (current == state_.positions_.end() ? Position::empty(trade.symbol()) : *current)
            .with_trade(trade, transaction.fees());
    if (!next_position) {
        return false;
    }

    // Stage all potentially allocating work. The final swaps cannot throw, so cash,
    // positions and the accepted history are committed as a single financial update.
    auto candidate = state_;
    if (current == state_.positions_.end()) {
        candidate.positions_.push_back(*next_position);
    } else {
        const auto index = static_cast<std::size_t>(current - state_.positions_.begin());
        candidate.positions_[index] = *next_position;
    }
    candidate.history_.push_back(transaction);
    candidate.cash_ = *next_cash;
    state_.swap(candidate);
    return true;
}

}  // namespace pql
