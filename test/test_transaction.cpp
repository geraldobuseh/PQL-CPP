#include <gtest/gtest.h>

#include <limits>
#include <type_traits>
#include <utility>

#include "domain/portfolio.hpp"

namespace {

using namespace pql;

static_assert(!std::is_default_constructible_v<Transaction> && !std::is_aggregate_v<Transaction>);
static_assert(std::is_copy_constructible_v<Transaction>);
static_assert(std::is_move_constructible_v<Transaction>);
static_assert(!std::is_copy_assignable_v<Transaction> && !std::is_move_assignable_v<Transaction>);
static_assert(!std::is_constructible_v<Transaction, PortfolioId, Trade, Money>);
static_assert(std::is_same_v<decltype(std::declval<Transaction&>().trade()), const Trade&>);
static_assert(std::is_same_v<decltype(std::declval<Transaction&>().symbol()), const Symbol&>);
static_assert(!std::is_invocable_v<decltype(&Transaction::create), PortfolioId, Trade, double>);
template <typename Type>
concept CanApplyTransaction =
    requires(Type& object, const Transaction& event) { object.applyTransaction(event); };
static_assert(CanApplyTransaction<Portfolio> && !CanApplyTransaction<PortfolioSnapshot>);

Trade fill(std::int64_t id, OrderSide side, double quantity, double price, std::int64_t tick = 0) {
    const Timestamp time{Timestamp::Value{std::chrono::milliseconds{tick}}};
    const auto order =
        Order::create_market(OrderId::create(id).value(), Symbol::create("AAPL").value(), side,
                             Quantity::create(quantity).value(), time);
    return Trade::create(order.value(), Price::create(price).value(), time).value();
}

Transaction event(std::int64_t id, OrderSide side, double quantity, double price, double fees = 0.0,
                  std::int64_t tick = 0, std::int64_t portfolio = 1) {
    return Transaction::create(PortfolioId::create(portfolio).value(),
                               fill(id, side, quantity, price, tick), Money::create(fees).value())
        .value();
}

Portfolio funded(double cash = 1000.0) {
    return Portfolio::create(PortfolioId::create(1).value(), Money::create(cash).value()).value();
}

std::vector<MarketPrice> marks(double price = 120.0) {
    return {{Symbol::create("AAPL").value(), Price::create(price).value()}};
}

void expect_money(const std::optional<Money>& actual, double expected) {
    ASSERT_TRUE(actual);
    EXPECT_DOUBLE_EQ(actual->value(), expected);
}

TEST(TransactionTest, OwnsAllFinancialFactsAndSurvivesSourceReplacement) {
    auto trade = fill(7, OrderSide::Buy, 2.0, 100.0, 42);
    const auto record =
        Transaction::create(PortfolioId::create(1).value(), trade, Money::create(2.0).value());
    ASSERT_TRUE(record);
    EXPECT_EQ(record->portfolio_id(), PortfolioId::create(1).value());
    EXPECT_EQ(record->order_id(), trade.order_id());
    EXPECT_EQ(record->symbol(), trade.symbol());
    EXPECT_EQ(record->side(), OrderSide::Buy);
    EXPECT_DOUBLE_EQ(record->quantity().value(), 2.0);
    EXPECT_DOUBLE_EQ(record->price().value(), 100.0);
    EXPECT_DOUBLE_EQ(record->fees().value(), 2.0);
    EXPECT_EQ(record->timestamp(), trade.timestamp());
    EXPECT_EQ(record->trade(), trade);
    const auto original = *record;
    trade = fill(8, OrderSide::Sell, 1.0, 120.0, 43);
    EXPECT_EQ(*record, original);
    auto copied = original;
    const auto moved = std::move(copied);
    EXPECT_EQ(moved, original);
    EXPECT_EQ(copied, original);
}

TEST(TransactionTest, FeesMustBeExplicitFiniteAndNonnegative) {
    const auto id = PortfolioId::create(1).value();
    const auto trade = fill(1, OrderSide::Buy, 2.0, 100.0);
    EXPECT_FALSE(Transaction::create(id, trade, Money::create(-1.0).value()));
    EXPECT_TRUE(Transaction::create(id, trade, Money::create(0.0).value()));
    EXPECT_TRUE(Transaction::create(id, trade, Money::create(2.0).value()));
    EXPECT_FALSE(Money::create(std::numeric_limits<double>::quiet_NaN()));
    EXPECT_FALSE(Money::create(std::numeric_limits<double>::infinity()));
    EXPECT_FALSE(Money::create(-std::numeric_limits<double>::infinity()));
}

TEST(TransactionReplayTest, TicketExampleReconstructsCashSharesProfitAndValue) {
    const std::vector<Transaction> history{event(1, OrderSide::Buy, 2.0, 100.0),
                                           event(2, OrderSide::Sell, 1.0, 120.0, 0.0, 1)};
    const auto portfolio =
        Portfolio::replay(PortfolioId::create(1).value(), Money::create(1000.0).value(), history);
    ASSERT_TRUE(portfolio);
    EXPECT_DOUBLE_EQ(portfolio->cashBalance().value(), 920.0);
    ASSERT_EQ(portfolio->positions().size(), 1U);
    const auto& position = portfolio->positions()[0];
    EXPECT_DOUBLE_EQ(position.quantity().value(), 1.0);
    EXPECT_DOUBLE_EQ(position.average_cost()->value(), 100.0);
    EXPECT_DOUBLE_EQ(position.realized_pnl().value(), 20.0);
    expect_money(position.cost_basis(), 100.0);
    expect_money(position.unrealized_pnl(Price::create(120.0).value()), 20.0);
    expect_money(portfolio->marketValue(marks()), 120.0);
    expect_money(portfolio->totalValue(marks()), 1040.0);
    EXPECT_EQ(portfolio->transactionHistory(), history);
    EXPECT_FALSE(portfolio->totalValue({}));  // History does not imply a current market mark.
    const auto again = Portfolio::replay(portfolio->id(), portfolio->startingCash(), history);
    ASSERT_TRUE(again);
    EXPECT_EQ(again->snapshot(), portfolio->snapshot());
}

TEST(TransactionReplayTest, FeesReconstructExactlyAndReconcileWithTotalProfit) {
    auto live = funded();
    const auto buy = event(1, OrderSide::Buy, 2.0, 100.0, 2.0);
    const auto sell = event(2, OrderSide::Sell, 1.0, 120.0, 1.0, 1);
    ASSERT_TRUE(live.applyTransaction(buy));
    EXPECT_DOUBLE_EQ(live.cashBalance().value(), 798.0);
    EXPECT_DOUBLE_EQ(live.positions()[0].average_cost()->value(), 101.0);
    expect_money(live.totalValue(marks(100.0)), 998.0);
    expect_money(live.totalValue(marks()), 1038.0);
    ASSERT_TRUE(live.applyTransaction(sell));
    EXPECT_DOUBLE_EQ(live.cashBalance().value(), 917.0);
    EXPECT_DOUBLE_EQ(live.positions()[0].quantity().value(), 1.0);
    EXPECT_DOUBLE_EQ(live.positions()[0].average_cost()->value(), 101.0);
    EXPECT_DOUBLE_EQ(live.positions()[0].realized_pnl().value(), 18.0);
    expect_money(live.positions()[0].unrealized_pnl(Price::create(120.0).value()), 19.0);
    expect_money(live.totalValue(marks()), 1037.0);
    EXPECT_EQ(live.transactionHistory(), (std::vector<Transaction>{buy, sell}));
    const auto replay =
        Portfolio::replay(live.id(), live.startingCash(), live.transactionHistory());
    ASSERT_TRUE(replay);
    EXPECT_EQ(replay->snapshot(), live.snapshot());
    // $37 total gain = $18 realized + $19 unrealized = $40 gross gain - $3 fees.
}

TEST(TransactionReplayTest, WeightedPurchasesAndPartialSalesIncludeTheirOwnFees) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 2.0)));
    ASSERT_TRUE(portfolio.applyTransaction(event(2, OrderSide::Buy, 1.0, 120.0, 2.0, 1)));
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].average_cost()->value(), 108.0);
    expect_money(portfolio.positions()[0].cost_basis(), 324.0);
    ASSERT_TRUE(portfolio.applyTransaction(event(3, OrderSide::Sell, 1.0, 130.0, 3.0, 2)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 803.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].quantity().value(), 2.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].average_cost()->value(), 108.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].realized_pnl().value(), 19.0);
    expect_money(portfolio.totalValue(marks(130.0)), 1063.0);
}

TEST(TransactionReplayTest, LiquidationRetainsFeeAdjustedRealizedHistory) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 2.0)));
    ASSERT_TRUE(portfolio.applyTransaction(event(2, OrderSide::Sell, 1.0, 120.0, 1.0, 1)));
    ASSERT_TRUE(portfolio.applyTransaction(event(3, OrderSide::Sell, 1.0, 90.0, 1.0, 2)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 1006.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].quantity().value(), 0.0);
    EXPECT_FALSE(portfolio.positions()[0].average_cost());
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].realized_pnl().value(), 6.0);
    expect_money(portfolio.totalValue({}), 1006.0);
}

TEST(TransactionReplayTest, RejectsWrongPortfolioAndConflictingEventsAtomically) {
    auto portfolio = funded();
    const auto before = portfolio.snapshot();
    const auto wrong = event(1, OrderSide::Buy, 2.0, 100.0, 2.0, 0, 2);
    EXPECT_FALSE(portfolio.applyTransaction(wrong));
    EXPECT_EQ(portfolio.snapshot(), before);
    EXPECT_FALSE(Portfolio::replay(portfolio.id(), portfolio.startingCash(), {wrong}));
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 2.0)));
    const auto accepted = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 3.0)));
    EXPECT_FALSE(portfolio.applyTransaction(event(2, OrderSide::Sell, 3.0, 100.0, 1.0, 1)));
    EXPECT_EQ(portfolio.snapshot(), accepted);
}

TEST(TransactionReplayTest, BuyFeeCanMakeAnOtherwiseAffordableFillReject) {
    auto portfolio = funded(200.0);
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 2.0)));
    EXPECT_EQ(portfolio.snapshot(), before);
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 0.0);
}

TEST(TransactionReplayTest, FeesEqualSaleProceedsStillApplyTheEvent) {
    auto portfolio = funded(200.0);
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTransaction(event(2, OrderSide::Sell, 1.0, 120.0, 120.0, 1)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 0.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].quantity().value(), 1.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].realized_pnl().value(), -100.0);
    EXPECT_EQ(portfolio.transactionHistory().size(), 2U);
}

TEST(TransactionReplayTest, FeesAboveProceedsRequireCashForNetDebit) {
    auto sufficient = funded();
    ASSERT_TRUE(sufficient.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0, 2.0)));
    ASSERT_TRUE(sufficient.applyTransaction(event(2, OrderSide::Sell, 1.0, 120.0, 121.0, 1)));
    EXPECT_DOUBLE_EQ(sufficient.cashBalance().value(), 797.0);
    EXPECT_DOUBLE_EQ(sufficient.positions()[0].realized_pnl().value(), -102.0);

    auto insufficient = funded(200.0);
    ASSERT_TRUE(insufficient.applyTransaction(event(1, OrderSide::Buy, 2.0, 100.0)));
    const auto before = insufficient.snapshot();
    EXPECT_FALSE(insufficient.applyTransaction(event(2, OrderSide::Sell, 1.0, 120.0, 121.0, 1)));
    EXPECT_EQ(insufficient.snapshot(), before);
}

TEST(TransactionReplayTest, FeesCanExactlyCancelGrossRealizedProfit) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTransaction(event(1, OrderSide::Buy, 1.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTransaction(event(2, OrderSide::Sell, 1.0, 120.0, 20.0, 1)));
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].realized_pnl().value(), 0.0);
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 1000.0);
}

TEST(TransactionReplayTest, RejectsOverflowAndFeesLostThroughRounding) {
    auto portfolio = funded(std::numeric_limits<double>::max());
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(
        portfolio.applyTransaction(event(1, OrderSide::Buy, 1.0, std::numeric_limits<double>::max(),
                                         std::numeric_limits<double>::max())));
    EXPECT_FALSE(portfolio.applyTransaction(event(2, OrderSide::Buy, 1.0, 1e20, 1.0)));
    EXPECT_EQ(portfolio.snapshot(), before);
    auto seller = funded(1e22);
    ASSERT_TRUE(seller.applyTransaction(event(1, OrderSide::Buy, 1.0, 1e20)));
    const auto seller_before = seller.snapshot();
    EXPECT_FALSE(seller.applyTransaction(event(2, OrderSide::Sell, 1.0, 1e20, 1.0, 1)));
    EXPECT_EQ(seller.snapshot(), seller_before);
    const auto position = Position::empty(Symbol::create("AAPL").value());
    EXPECT_FALSE(
        position.with_trade(fill(1, OrderSide::Buy, 1.0, 100.0), Money::create(-1.0).value()));
}

TEST(TransactionReplayTest, LegacyApplyTradeCreatesOnlyZeroFeeAttributedTransactions) {
    auto portfolio = funded();
    const auto trade = fill(7, OrderSide::Buy, 2.0, 100.0);
    ASSERT_TRUE(portfolio.applyTrade(trade));
    ASSERT_EQ(portfolio.transactionHistory().size(), 1U);
    const auto& record = portfolio.transactionHistory()[0];
    EXPECT_EQ(record.portfolio_id(), portfolio.id());
    EXPECT_EQ(record.trade(), trade);
    EXPECT_DOUBLE_EQ(record.fees().value(), 0.0);
    EXPECT_FALSE(portfolio.applyTransaction(event(7, OrderSide::Buy, 2.0, 100.0, 2.0)));
    const auto replay =
        Portfolio::replay(portfolio.id(), portfolio.startingCash(), portfolio.transactionHistory());
    ASSERT_TRUE(replay);
    EXPECT_EQ(portfolio.snapshot(), replay->snapshot());
}

}  // namespace
