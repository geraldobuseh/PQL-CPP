#pragma once

#include <optional>

#include "domain/order.hpp"

namespace pql {

// A full-fill simulation record. Construction does not execute an order, update
// its status, authorize a sell, or guarantee that an order has not already filled.
// The execution layer must establish those facts before recording a fill.
class Trade {
   public:
    [[nodiscard]] static std::optional<Trade> create(const Order& order, Price price,
                                                     Timestamp timestamp);

    [[nodiscard]] OrderId order_id() const noexcept { return order_id_; }
    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] OrderSide side() const noexcept { return side_; }
    [[nodiscard]] Quantity quantity() const noexcept { return quantity_; }
    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Timestamp timestamp() const noexcept { return timestamp_; }
    bool operator==(const Trade&) const = default;

   private:
    Trade(const Order& order, Price price, Timestamp timestamp);

    OrderId order_id_;
    Symbol symbol_;
    OrderSide side_;
    Quantity quantity_;
    Price price_;
    Timestamp timestamp_;
};

}  // namespace pql
