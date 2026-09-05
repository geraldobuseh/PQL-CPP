#include "domain/financial_types.hpp"

#include <algorithm>
#include <cmath>

namespace pql {
namespace {

bool is_ascii_alphanumeric(char character) {
    return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9');
}

}  // namespace

std::optional<Symbol> Symbol::create(std::string_view value) {
    if (value.empty() || !is_ascii_alphanumeric(value.front()) ||
        !std::all_of(value.begin(), value.end(), [](char character) {
            return is_ascii_alphanumeric(character) || character == '.' || character == '-';
        })) {
        return std::nullopt;
    }
    return Symbol{value};
}

std::optional<Money> Money::create(double value) {
    if (!std::isfinite(value)) {
        return std::nullopt;
    }
    return Money{value};
}

std::optional<Quantity> Quantity::create(double value) {
    if (!std::isfinite(value) || value < 0.0) {
        return std::nullopt;
    }
    return Quantity{value};
}

std::optional<Price> Price::create(double value) {
    if (!std::isfinite(value) || value <= 0.0) {
        return std::nullopt;
    }
    return Price{value};
}

std::optional<OrderId> OrderId::create(std::int64_t value) {
    if (value <= 0) {
        return std::nullopt;
    }
    return OrderId{value};
}

std::optional<PortfolioId> PortfolioId::create(std::int64_t value) {
    if (value <= 0) {
        return std::nullopt;
    }
    return PortfolioId{value};
}

std::optional<StrategyId> StrategyId::create(std::int64_t value) {
    if (value <= 0) {
        return std::nullopt;
    }
    return StrategyId{value};
}

}  // namespace pql
