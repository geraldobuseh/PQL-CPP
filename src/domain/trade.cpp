#include "domain/trade.hpp"

namespace pql {

Trade::Trade(const Order& order, Price price, Timestamp timestamp)
    : order_id_(order.id()),
      symbol_(order.symbol()),
      side_(order.side()),
      quantity_(order.quantity()),
      price_(price),
      timestamp_(timestamp) {}

std::optional<Trade> Trade::create(const Order& order, Price price, Timestamp timestamp) {
    if (timestamp < order.timestamp()) {
        return std::nullopt;
    }
    return Trade{order, price, timestamp};
}

}  // namespace pql
