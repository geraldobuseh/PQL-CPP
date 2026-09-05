#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>

#include "domain/position.hpp"

namespace {

using namespace pql;

static_assert(!std::is_default_constructible_v<Position> && !std::is_aggregate_v<Position>);
static_assert(!std::is_invocable_v<decltype(&Position::market_value), Position, double>);
static_assert(!std::is_invocable_v<decltype(&Position::with_trade), Position, Order>);

// All arguments in these fixtures are valid Order/Trade inputs. value() makes a
// broken fixture fail the test rather than allowing an unchecked dereference.
Trade fill(OrderSide side, double quantity, double price, std::int64_t tick = 0,
           const char* symbol = "SPY", std::int64_t id = 1) {
    const Timestamp time{Timestamp::Value{std::chrono::milliseconds{tick}}};
    const auto order =
        Order::create_market(OrderId::create(id).value(), Symbol::create(symbol).value(), side,
                             Quantity::create(quantity).value(), time);
    return Trade::create(order.value(), Price::create(price).value(), time).value();
}

Position empty() { return Position::empty(Symbol::create("SPY").value()); }

Position bought(double quantity, double price) {
    return empty().with_trade(fill(OrderSide::Buy, quantity, price)).value();
}

Position two_buys() {
    return bought(10.0, 100.0).with_trade(fill(OrderSide::Buy, 10.0, 120.0, 1, "SPY", 2)).value();
}

void expect_value(const std::optional<Money>& amount, double expected) {
    ASSERT_TRUE(amount);
    EXPECT_DOUBLE_EQ(amount->value(), expected);
}

TEST(PositionTest, EmptyHasNoAverageAndZeroValue) {
    const auto position = empty();
    EXPECT_EQ(position.symbol(), Symbol::create("SPY").value());
    EXPECT_DOUBLE_EQ(position.quantity().value(), 0.0);
    EXPECT_FALSE(position.average_cost());
    EXPECT_DOUBLE_EQ(position.realized_pnl().value(), 0.0);
    expect_value(position.cost_basis(), 0.0);
    expect_value(position.market_value(Price::create(125.0).value()), 0.0);
    expect_value(position.unrealized_pnl(Price::create(125.0).value()), 0.0);
}

TEST(PositionTest, FirstPurchaseEstablishesQuantityAndCost) {
    const auto original = empty();
    const auto position = original.with_trade(fill(OrderSide::Buy, 10.0, 100.0));
    ASSERT_TRUE(position);
    EXPECT_DOUBLE_EQ(position->quantity().value(), 10.0);
    ASSERT_TRUE(position->average_cost());
    EXPECT_DOUBLE_EQ(position->average_cost()->value(), 100.0);
    expect_value(position->cost_basis(), 1000.0);
    expect_value(position->market_value(Price::create(125.0).value()), 1250.0);
    expect_value(position->unrealized_pnl(Price::create(125.0).value()), 250.0);
    EXPECT_DOUBLE_EQ(position->realized_pnl().value(), 0.0);
    EXPECT_EQ(original, empty());
}

TEST(PositionTest, AdditionalPurchaseRecalculatesWeightedAverage) {
    const auto position = two_buys();
    EXPECT_DOUBLE_EQ(position.quantity().value(), 20.0);
    ASSERT_TRUE(position.average_cost());
    EXPECT_DOUBLE_EQ(position.average_cost()->value(), 110.0);
    expect_value(position.cost_basis(), 2200.0);
    expect_value(position.market_value(Price::create(125.0).value()), 2500.0);
    expect_value(position.unrealized_pnl(Price::create(125.0).value()), 300.0);
}

TEST(PositionTest, UnequalPurchasesWeightBySharesRatherThanTradeCount) {
    const auto position = bought(10.0, 100.0).with_trade(fill(OrderSide::Buy, 30.0, 120.0, 1));
    ASSERT_TRUE(position);
    EXPECT_DOUBLE_EQ(position->quantity().value(), 40.0);
    ASSERT_TRUE(position->average_cost());
    EXPECT_DOUBLE_EQ(position->average_cost()->value(), 115.0);
    expect_value(position->cost_basis(), 4600.0);
}

TEST(PositionTest, PartialSalePreservesAverageAndRealizesOnlySoldShares) {
    const auto original = two_buys();
    const auto position = original.with_trade(fill(OrderSide::Sell, 5.0, 130.0, 2, "SPY", 3));
    ASSERT_TRUE(position);
    EXPECT_DOUBLE_EQ(position->quantity().value(), 15.0);
    EXPECT_EQ(position->average_cost(), original.average_cost());
    expect_value(position->cost_basis(), 1650.0);
    EXPECT_DOUBLE_EQ(position->realized_pnl().value(), 100.0);
    expect_value(position->market_value(Price::create(125.0).value()), 1875.0);
    expect_value(position->unrealized_pnl(Price::create(125.0).value()), 225.0);
    EXPECT_EQ(original, two_buys());
}

TEST(PositionTest, CompleteLiquidationClearsBasisAndRetainsRealizedHistory) {
    const auto partial = two_buys().with_trade(fill(OrderSide::Sell, 5.0, 130.0, 2)).value();
    const auto position = partial.with_trade(fill(OrderSide::Sell, 15.0, 90.0, 3));
    ASSERT_TRUE(position);
    EXPECT_DOUBLE_EQ(position->quantity().value(), 0.0);
    EXPECT_FALSE(position->average_cost());
    expect_value(position->cost_basis(), 0.0);
    expect_value(position->market_value(Price::create(125.0).value()), 0.0);
    expect_value(position->unrealized_pnl(Price::create(125.0).value()), 0.0);
    EXPECT_DOUBLE_EQ(position->realized_pnl().value(), -200.0);
    // Proceeds 5*130 + 15*90 minus purchase cost 10*100 + 10*120 = -200.
}

TEST(PositionTest, ReopeningUsesNewCostAndPreservesRealizedHistory) {
    const auto flat = bought(10.0, 100.0).with_trade(fill(OrderSide::Sell, 10.0, 90.0, 1)).value();
    const auto position = flat.with_trade(fill(OrderSide::Buy, 2.0, 50.0, 2));
    ASSERT_TRUE(position);
    EXPECT_DOUBLE_EQ(position->quantity().value(), 2.0);
    ASSERT_TRUE(position->average_cost());
    EXPECT_DOUBLE_EQ(position->average_cost()->value(), 50.0);
    expect_value(position->cost_basis(), 100.0);
    EXPECT_DOUBLE_EQ(position->realized_pnl().value(), -100.0);
}

TEST(PositionTest, FractionalSharesAndUnrealizedLoss) {
    const auto position =
        bought(0.5, 100.0).with_trade(fill(OrderSide::Buy, 0.25, 160.0, 1)).value();
    ASSERT_TRUE(position.average_cost());
    EXPECT_DOUBLE_EQ(position.average_cost()->value(), 120.0);
    expect_value(position.cost_basis(), 90.0);
    expect_value(position.unrealized_pnl(Price::create(80.0).value()), -30.0);
    const auto sold = position.with_trade(fill(OrderSide::Sell, 0.25, 80.0, 2));
    ASSERT_TRUE(sold);
    EXPECT_DOUBLE_EQ(sold->quantity().value(), 0.5);
    EXPECT_DOUBLE_EQ(sold->realized_pnl().value(), -10.0);
    expect_value(sold->cost_basis(), 60.0);
}

TEST(PositionTest, MarkChangesDoNotChangeCostOrRealizedProfit) {
    const auto position = bought(10.0, 100.0);
    const auto snapshot = position;
    expect_value(position.unrealized_pnl(Price::create(100.0).value()), 0.0);
    expect_value(position.unrealized_pnl(Price::create(90.0).value()), -100.0);
    expect_value(position.unrealized_pnl(Price::create(120.0).value()), 200.0);
    EXPECT_EQ(position, snapshot);
}

TEST(PositionTest, RejectsWrongSymbolAndOversellsWithoutChangingSource) {
    const auto position = bought(1.0, 100.0);
    const auto snapshot = position;
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Buy, 1.0, 100.0, 1, "AAPL")));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, 1.0, 100.0, 1, "spy")));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, 2.0, 100.0, 1)));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, std::nextafter(1.0, 2.0), 100.0, 1)));
    EXPECT_FALSE(empty().with_trade(fill(OrderSide::Sell, 1.0, 100.0)));
    EXPECT_EQ(position, snapshot);
}

TEST(PositionTest, RejectsOlderFillsButReplaysEqualTimestampsInCallerOrder) {
    const auto position = bought(1.0, 100.0);
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Buy, 1.0, 120.0, -1)));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, 1.0, 120.0, -1)));
    const auto same_time = position.with_trade(fill(OrderSide::Buy, 1.0, 120.0));
    ASSERT_TRUE(same_time);
    EXPECT_DOUBLE_EQ(same_time->average_cost()->value(), 110.0);
    const auto flat = position.with_trade(fill(OrderSide::Sell, 1.0, 120.0, 5)).value();
    EXPECT_FALSE(flat.with_trade(fill(OrderSide::Buy, 1.0, 120.0, 4)));
}

TEST(PositionTest, ReplayingSameOrderedFillsIsDeterministic) {
    const Trade trades[] = {fill(OrderSide::Buy, 10.0, 100.0, 0, "SPY", 1),
                            fill(OrderSide::Buy, 10.0, 120.0, 1, "SPY", 2),
                            fill(OrderSide::Sell, 5.0, 130.0, 2, "SPY", 3),
                            fill(OrderSide::Sell, 15.0, 90.0, 3, "SPY", 4)};
    auto first = empty();
    auto second = empty();
    for (const auto& trade : trades) {
        first = first.with_trade(trade).value();
        second = second.with_trade(trade).value();
    }
    EXPECT_EQ(first, second);
    EXPECT_DOUBLE_EQ(first.realized_pnl().value(), -200.0);
}

TEST(PositionTest, RejectsOverflowInNotionalQuantityAndCombinedBasis) {
    const double maximum = std::numeric_limits<double>::max();
    EXPECT_FALSE(empty().with_trade(fill(OrderSide::Buy, maximum, 2.0)));
    const auto huge_quantity = bought(maximum, std::numeric_limits<double>::min());
    EXPECT_FALSE(huge_quantity.with_trade(
        fill(OrderSide::Buy, maximum, std::numeric_limits<double>::min(), 1)));
    const auto huge_basis = bought(1.0, maximum);
    EXPECT_FALSE(huge_basis.with_trade(fill(OrderSide::Buy, 1.0, maximum, 1)));
    EXPECT_FALSE(huge_basis.with_trade(fill(OrderSide::Sell, 1.0, maximum, 1, "AAPL")));
    EXPECT_EQ(huge_basis, bought(1.0, maximum));
}

TEST(PositionTest, RejectsUnrepresentableValuationAndSellNotional) {
    const auto position = bought(std::numeric_limits<double>::max(), 1.0);
    EXPECT_FALSE(position.market_value(Price::create(2.0).value()));
    EXPECT_FALSE(position.unrealized_pnl(Price::create(2.0).value()));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, position.quantity().value(), 2.0, 1)));
}

TEST(PositionTest, RejectsPositiveQuantityChangesThatRoundAway) {
    const auto position = bought(1e20, 1.0);
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Buy, 1.0, 1.0, 1)));
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, 1.0, 1.0, 1)));
    EXPECT_EQ(position, bought(1e20, 1.0));
}

TEST(PositionTest, RejectsPositiveCostThatRoundsAwayInAddition) {
    const auto position = bought(1.0, 1e20);
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Buy, 1.0, 1.0, 1)));
}

TEST(PositionTest, RejectsUnderflowRatherThanErasingValue) {
    const double tiny = std::numeric_limits<double>::min();
    EXPECT_FALSE(empty().with_trade(fill(OrderSide::Buy, tiny, tiny)));
    const auto position = bought(tiny, 1.0);
    EXPECT_FALSE(position.market_value(Price::create(tiny).value()));
    EXPECT_FALSE(position.unrealized_pnl(Price::create(tiny).value()));
}

TEST(PositionTest, RejectsRealizedOverflowAndSwallowedProfitContributions) {
    const double large_profit = std::numeric_limits<double>::max() * 0.75;
    const auto flat =
        bought(1.0, 1.0).with_trade(fill(OrderSide::Sell, 1.0, large_profit, 1)).value();
    const auto reopened = flat.with_trade(fill(OrderSide::Buy, 1.0, 1.0, 2)).value();
    EXPECT_FALSE(reopened.with_trade(fill(OrderSide::Sell, 1.0, large_profit, 3)));

    for (const auto first_price : {2.0, 1e20}) {
        const auto first_sale =
            bought(1.0, 1.0).with_trade(fill(OrderSide::Sell, 1.0, first_price, 1)).value();
        const auto next = first_sale.with_trade(fill(OrderSide::Buy, 1.0, 1.0, 2)).value();
        const double second_price = first_price == 2.0 ? 1e20 : 2.0;
        EXPECT_FALSE(next.with_trade(fill(OrderSide::Sell, 1.0, second_price, 3)));
    }
}

TEST(PositionTest, RejectsUnderflowOfProfitOrRemainingBasis) {
    const double smallest = std::numeric_limits<double>::denorm_min();
    const auto tiny_shares = bought(smallest, 1.0);
    const double slightly_higher = std::nextafter(1.0, 2.0);
    EXPECT_FALSE(tiny_shares.unrealized_pnl(Price::create(slightly_higher).value()));
    EXPECT_FALSE(tiny_shares.with_trade(fill(OrderSide::Sell, smallest, slightly_higher, 1)));

    const auto tiny_basis = bought(1.0, smallest);
    EXPECT_FALSE(tiny_basis.with_trade(fill(OrderSide::Sell, 0.75, 1.0, 1)));
}

TEST(PositionTest, FractionalRemaindersAreNotSilentlyLiquidated) {
    const auto position =
        bought(0.3, 100.0).with_trade(fill(OrderSide::Sell, 0.1, 100.0, 1)).value();
    EXPECT_FALSE(position.with_trade(fill(OrderSide::Sell, 0.2, 100.0, 2)));
    const auto flat =
        position.with_trade(fill(OrderSide::Sell, position.quantity().value(), 100.0, 2));
    ASSERT_TRUE(flat);
    EXPECT_DOUBLE_EQ(flat->quantity().value(), 0.0);
    EXPECT_FALSE(flat->average_cost());
}

}  // namespace
