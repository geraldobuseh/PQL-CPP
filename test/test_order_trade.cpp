#include <gtest/gtest.h>

#include <limits>
#include <type_traits>
#include <utility>

#include "domain/order.hpp"
#include "domain/trade.hpp"

namespace {

using namespace pql;

static_assert(!std::is_default_constructible_v<Order> && !std::is_aggregate_v<Order>);
static_assert(!std::is_default_constructible_v<Trade> && !std::is_aggregate_v<Trade>);
static_assert(!std::is_constructible_v<Order, OrderId, Symbol, OrderSide, Quantity, Timestamp>);
static_assert(!std::is_constructible_v<Trade, Order, Price, Timestamp>);
static_assert(!std::is_convertible_v<Order, Trade>);
static_assert(!std::is_invocable_v<decltype(&Order::create_market), OrderId, Symbol, OrderSide,
                                   double, Timestamp>);
static_assert(!std::is_invocable_v<decltype(&Trade::create), Order, double, Timestamp>);
static_assert(std::is_same_v<decltype(std::declval<Order&>().symbol()), const Symbol&>);
static_assert(std::is_same_v<decltype(std::declval<Trade&>().symbol()), const Symbol&>);

Timestamp at(std::int64_t milliseconds) {
    return Timestamp{Timestamp::Value{std::chrono::milliseconds{milliseconds}}};
}

class OrderTradeTest : public ::testing::TestWithParam<OrderSide> {
   protected:
    const OrderId id = *OrderId::create(7);
    const Symbol symbol = *Symbol::create("SPY");
    const Quantity quantity = *Quantity::create(0.5);
    const Price price = *Price::create(500.25);
    const Timestamp submitted = at(1000);
};

TEST_P(OrderTradeTest, CreatesPendingMarketRequest) {
    const auto order = Order::create_market(id, symbol, GetParam(), quantity, submitted);
    ASSERT_TRUE(order);
    EXPECT_EQ(order->id(), id);
    EXPECT_EQ(order->symbol(), symbol);
    EXPECT_EQ(order->side(), GetParam());
    EXPECT_EQ(order->quantity(), quantity);
    EXPECT_EQ(order->type(), OrderType::Market);
    EXPECT_EQ(order->timestamp(), submitted);
    EXPECT_EQ(order->status(), OrderStatus::Pending);
    EXPECT_EQ(order, Order::create_market(id, symbol, GetParam(), quantity, submitted));
}

TEST_P(OrderTradeTest, RejectsBothSignsOfZeroQuantity) {
    for (const double input : {0.0, -0.0}) {
        const auto zero = Quantity::create(input);
        ASSERT_TRUE(zero);  // Valid holding magnitude, invalid order quantity.
        EXPECT_FALSE(Order::create_market(id, symbol, GetParam(), *zero, submitted));
    }
}

TEST_P(OrderTradeTest, InvalidPrimitiveQuantitiesCannotReachOrderOrTrade) {
    for (const double input :
         {-1.0, -std::numeric_limits<double>::max(), std::numeric_limits<double>::quiet_NaN(),
          std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()}) {
        const auto invalid = Quantity::create(input);
        ASSERT_FALSE(invalid);
        // Compose the actual validation boundaries without dereferencing a rejection.
        const auto order = invalid
                               ? Order::create_market(id, symbol, GetParam(), *invalid, submitted)
                               : std::nullopt;
        EXPECT_FALSE(order);
        const auto trade = order ? Trade::create(*order, price, submitted) : std::nullopt;
        EXPECT_FALSE(trade);
    }
}

TEST_P(OrderTradeTest, TradePreservesRequestFieldsAndRecordsExecution) {
    const auto order = Order::create_market(id, symbol, GetParam(), quantity, submitted);
    ASSERT_TRUE(order);
    const auto trade = Trade::create(*order, price, at(1001));
    ASSERT_TRUE(trade);
    EXPECT_EQ(trade->order_id(), id);
    EXPECT_EQ(trade->symbol(), symbol);
    EXPECT_EQ(trade->side(), GetParam());
    EXPECT_EQ(trade->quantity(), quantity);
    EXPECT_EQ(trade->price(), price);
    EXPECT_EQ(trade->timestamp(), at(1001));
    EXPECT_EQ(trade, Trade::create(*order, price, at(1001)));
    EXPECT_NE(trade, Trade::create(*order, price, at(1002)));
    EXPECT_EQ(order->status(), OrderStatus::Pending);  // Record creation is not execution.
}

TEST_P(OrderTradeTest, TradeCannotPrecedeSubmission) {
    const auto order = Order::create_market(id, symbol, GetParam(), quantity, submitted);
    ASSERT_TRUE(order);
    EXPECT_FALSE(Trade::create(*order, price, at(999)));
    EXPECT_TRUE(Trade::create(*order, price, submitted));
    EXPECT_TRUE(Trade::create(*order, price, at(1001)));
}

TEST_P(OrderTradeTest, OwnsValuesIndependentlyOfSourceLifetimesAndMoves) {
    auto source_symbol = *Symbol::create("SPY");
    auto order = Order::create_market(id, source_symbol, GetParam(), quantity, submitted);
    ASSERT_TRUE(order);
    source_symbol = *Symbol::create("AAPL");
    EXPECT_EQ(order->symbol(), symbol);
    const auto trade = Trade::create(*order, price, submitted);
    ASSERT_TRUE(trade);
    const auto moved_order = std::move(*order);
    EXPECT_EQ(moved_order.symbol(), symbol);
    EXPECT_EQ(order->symbol(), symbol);
    order.reset();
    EXPECT_EQ(trade->symbol(), symbol);
    EXPECT_EQ(trade->quantity(), quantity);
    EXPECT_EQ(trade->order_id(), id);
    auto trade_copy = *trade;
    const auto moved_trade = std::move(trade_copy);
    EXPECT_EQ(moved_trade, *trade);
    EXPECT_EQ(trade_copy, *trade);
}

INSTANTIATE_TEST_SUITE_P(BuyAndSell, OrderTradeTest,
                         ::testing::Values(OrderSide::Buy, OrderSide::Sell));

TEST(OrderValidationTest, RejectsUnsupportedSideValues) {
    for (const auto side :
         {static_cast<OrderSide>(-1), static_cast<OrderSide>(2), static_cast<OrderSide>(99)}) {
        EXPECT_FALSE(Order::create_market(*OrderId::create(1), *Symbol::create("SPY"), side,
                                          *Quantity::create(1.0), at(0)));
    }
}

TEST(OrderValidationTest, AllowsPositiveFiniteQuantityBoundsWithoutCalculatingNotional) {
    for (const double input :
         {std::numeric_limits<double>::min(), std::numeric_limits<double>::max()}) {
        const auto order = Order::create_market(*OrderId::create(1), *Symbol::create("SPY"),
                                                OrderSide::Buy, *Quantity::create(input), at(0));
        ASSERT_TRUE(order);
        const auto trade = Trade::create(*order, *Price::create(2.0), at(0));
        ASSERT_TRUE(trade);
        EXPECT_DOUBLE_EQ(trade->quantity().value(), input);
    }
}

TEST(TradeValidationTest, UsesExplicitInstantsIncludingBeforeEpoch) {
    const auto order = Order::create_market(*OrderId::create(1), *Symbol::create("SPY"),
                                            OrderSide::Buy, *Quantity::create(1.0), at(-2));
    ASSERT_TRUE(order);
    EXPECT_FALSE(Trade::create(*order, *Price::create(1.0), at(-3)));
    const auto trade = Trade::create(*order, *Price::create(1.0), at(-1));
    ASSERT_TRUE(trade);
    EXPECT_EQ(trade->timestamp(), at(-1));
}

}  // namespace
