#pragma once

#include <optional>

#include "domain/financial_types.hpp"

namespace pql {

enum class OrderSide { Buy, Sell };
enum class OrderType { Market };
enum class OrderStatus { Pending };

// A market-style simulated request, not authorization to execute.
// Sell requests require a holdings check in the future execution layer.
// This ticket models creation only; status transitions belong to execution.
class Order {
   public:
    [[nodiscard]] static std::optional<Order> create_market(OrderId id, const Symbol& symbol,
                                                            OrderSide side, Quantity quantity,
                                                            Timestamp timestamp);

    [[nodiscard]] OrderId id() const noexcept { return id_; }
    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] OrderSide side() const noexcept { return side_; }
    [[nodiscard]] Quantity quantity() const noexcept { return quantity_; }
    [[nodiscard]] OrderType type() const noexcept { return type_; }
    [[nodiscard]] Timestamp timestamp() const noexcept { return timestamp_; }
    [[nodiscard]] OrderStatus status() const noexcept { return status_; }
    bool operator==(const Order&) const = default;

   private:
    Order(OrderId id, const Symbol& symbol, OrderSide side, Quantity quantity, Timestamp timestamp);

    OrderId id_;
    Symbol symbol_;
    OrderSide side_;
    Quantity quantity_;
    OrderType type_{OrderType::Market};
    Timestamp timestamp_;
    OrderStatus status_{OrderStatus::Pending};
};

}  // namespace pql
