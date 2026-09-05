#include "domain/order.hpp"

namespace pql {

Order::Order(OrderId id, const Symbol& symbol, OrderSide side, Quantity quantity,
             Timestamp timestamp)
    : id_(id), symbol_(symbol), side_(side), quantity_(quantity), timestamp_(timestamp) {}

std::optional<Order> Order::create_market(OrderId id, const Symbol& symbol, OrderSide side,
                                          Quantity quantity, Timestamp timestamp) {
    if ((side != OrderSide::Buy && side != OrderSide::Sell) || quantity.value() <= 0.0) {
        return std::nullopt;
    }
    return Order{id, symbol, side, quantity, timestamp};
}

}  // namespace pql
