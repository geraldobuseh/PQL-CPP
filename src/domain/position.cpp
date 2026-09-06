#include "domain/position.hpp"

#include <cmath>

namespace pql {
namespace {

// Used only with positive factors. A zero product would silently erase value.
std::optional<Money> positive_product(double left, double right) {
    const double product = left * right;
    if (product <= 0.0) {
        return std::nullopt;
    }
    return Money::create(product);
}

std::optional<Money> price_difference_pnl(double quantity, double price, double average) {
    const double difference = price - average;
    const double pnl = quantity * difference;
    if (difference != 0.0 && pnl == 0.0) {
        return std::nullopt;
    }
    return Money::create(pnl);
}

}  // namespace

Position::Position(const Symbol& symbol, Quantity quantity, std::optional<Price> average_cost,
                   Money realized_pnl, std::optional<Timestamp> last_trade)
    : symbol_(symbol),
      quantity_(quantity),
      average_cost_(average_cost),
      realized_pnl_(realized_pnl),
      last_trade_(last_trade) {}

Position Position::empty(const Symbol& symbol) {
    // Zero is valid for these primitives; no external input is being dereferenced.
    return Position{symbol, *Quantity::create(0.0), std::nullopt, *Money::create(0.0),
                    std::nullopt};
}

std::optional<Money> Position::cost_basis() const {
    if (!average_cost_) {
        return Money::create(0.0);
    }
    return positive_product(quantity_.value(), average_cost_->value());
}

std::optional<Money> Position::market_value(Price mark) const {
    if (quantity_.value() == 0.0) {
        return Money::create(0.0);
    }
    return positive_product(quantity_.value(), mark.value());
}

std::optional<Money> Position::unrealized_pnl(Price mark) const {
    if (!average_cost_) {
        return Money::create(0.0);
    }
    if (!market_value(mark)) {
        return std::nullopt;
    }
    return price_difference_pnl(quantity_.value(), mark.value(), average_cost_->value());
}

std::optional<Position> Position::with_trade(const Trade& trade) const {
    return with_trade(trade, *Money::create(0.0));
}

std::optional<Position> Position::with_trade(const Trade& trade, Money fees) const {
    if (fees.value() < 0.0 || trade.symbol() != symbol_ ||
        (last_trade_ && trade.timestamp() < *last_trade_)) {
        return std::nullopt;
    }

    const double owned = quantity_.value();
    const double filled = trade.quantity().value();
    const auto notional = positive_product(filled, trade.price().value());
    if (!notional) {
        return std::nullopt;
    }

    double next_quantity = owned;
    auto next_average = average_cost_;
    auto next_realized = realized_pnl_;

    if (trade.side() == OrderSide::Buy) {
        const double acquisition_cost = notional->value() + fees.value();
        if (!std::isfinite(acquisition_cost) ||
            (fees.value() > 0.0 && acquisition_cost <= notional->value()) ||
            acquisition_cost <= fees.value()) {
            return std::nullopt;
        }
        next_quantity = owned + filled;
        // Reject a positive contribution lost in floating-point addition.
        if (!std::isfinite(next_quantity) || next_quantity <= owned ||
            (owned > 0.0 && next_quantity <= filled)) {
            return std::nullopt;
        }
        if (!average_cost_) {
            next_average = fees.value() == 0.0 ? std::optional<Price>{trade.price()}
                                               : Price::create(acquisition_cost / next_quantity);
            if (!next_average) {
                return std::nullopt;
            }
        } else {
            const auto basis = cost_basis();
            if (!basis) {
                return std::nullopt;
            }
            const double total_basis = basis->value() + acquisition_cost;
            if (!std::isfinite(total_basis) || total_basis <= basis->value() ||
                total_basis <= acquisition_cost) {
                return std::nullopt;
            }
            next_average = Price::create(total_basis / next_quantity);
            if (!next_average) {
                return std::nullopt;
            }
        }
    } else {
        if (!average_cost_ || filled > owned) {
            return std::nullopt;
        }
        const auto profit =
            price_difference_pnl(filled, trade.price().value(), average_cost_->value());
        if (!profit) {
            return std::nullopt;
        }
        const double net_profit = profit->value() - fees.value();
        if ((fees.value() > 0.0 && net_profit == profit->value()) ||
            (profit->value() != 0.0 && net_profit == -fees.value())) {
            return std::nullopt;
        }
        const double total_realized = realized_pnl_.value() + net_profit;
        if ((net_profit != 0.0 && total_realized == realized_pnl_.value()) ||
            (realized_pnl_.value() != 0.0 && total_realized == net_profit)) {
            return std::nullopt;
        }
        const auto realized = Money::create(total_realized);
        if (!realized) {
            return std::nullopt;
        }
        next_realized = *realized;
        if (filled == owned) {
            next_quantity = 0.0;
            next_average.reset();
        } else {
            next_quantity = owned - filled;
            if (next_quantity <= 0.0 || next_quantity >= owned) {
                return std::nullopt;
            }
        }
    }

    if (next_average && !positive_product(next_quantity, next_average->value())) {
        return std::nullopt;
    }
    const auto quantity = Quantity::create(next_quantity);
    if (!quantity) {
        return std::nullopt;
    }
    return Position{symbol_, *quantity, next_average, next_realized, trade.timestamp()};
}

}  // namespace pql
