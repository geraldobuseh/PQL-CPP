#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

#include "domain/portfolio.hpp"

namespace {

using namespace pql;

// These checks specify the strategy capability boundary, not just constness of a
// particular call site. A snapshot never names or converts to a mutation authority.
template <typename Type>
concept CanApplyTrade = requires(Type& object, const Trade& trade) { object.applyTrade(trade); };

static_assert(CanApplyTrade<Portfolio>);
static_assert(!CanApplyTrade<PortfolioSnapshot>);
static_assert(!std::is_convertible_v<PortfolioSnapshot&, Portfolio&>);
static_assert(!std::is_invocable_v<decltype(&Portfolio::applyTrade), PortfolioSnapshot&, Trade>);
static_assert(!std::is_invocable_v<decltype(&Portfolio::applyTrade), Portfolio&, Order>);
static_assert(!std::is_default_constructible_v<Portfolio> && !std::is_aggregate_v<Portfolio>);
static_assert(!std::is_default_constructible_v<PortfolioSnapshot>);
static_assert(std::is_same_v<decltype(std::declval<PortfolioSnapshot&>().positions()),
                             const std::vector<Position>&>);
static_assert(std::is_same_v<decltype(std::declval<PortfolioSnapshot&>().transactionHistory()),
                             const std::vector<Transaction>&>);
static_assert(
    std::is_same_v<decltype(std::declval<Portfolio&>().positions()), const std::vector<Position>&>);
static_assert(std::is_same_v<decltype(std::declval<Portfolio&>().transactionHistory()),
                             const std::vector<Transaction>&>);

Trade fill(std::int64_t id, const char* symbol, OrderSide side, double quantity, double price,
           std::int64_t tick = 0) {
    const Timestamp time{Timestamp::Value{std::chrono::milliseconds{tick}}};
    const auto order =
        Order::create_market(OrderId::create(id).value(), Symbol::create(symbol).value(), side,
                             Quantity::create(quantity).value(), time);
    return Trade::create(order.value(), Price::create(price).value(), time).value();
}

Portfolio funded(double cash = 10000.0) {
    return Portfolio::create(PortfolioId::create(1).value(), Money::create(cash).value()).value();
}

Transaction record(const Trade& trade) {
    return Transaction::create(PortfolioId::create(1).value(), trade, Money::create(0.0).value())
        .value();
}

MarketPrice mark(const char* symbol, double price) {
    return {Symbol::create(symbol).value(), Price::create(price).value()};
}

void expect_amount(const std::optional<Money>& value, double expected) {
    ASSERT_TRUE(value);
    EXPECT_DOUBLE_EQ(value->value(), expected);
}

TEST(PortfolioTest, StartsWithNonnegativeCashAndEmptyHistory) {
    const auto portfolio = funded();
    EXPECT_EQ(portfolio.id(), PortfolioId::create(1).value());
    EXPECT_DOUBLE_EQ(portfolio.startingCash().value(), 10000.0);
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 10000.0);
    EXPECT_TRUE(portfolio.positions().empty());
    EXPECT_TRUE(portfolio.transactionHistory().empty());
    expect_amount(portfolio.marketValue({}), 0.0);
    expect_amount(portfolio.totalValue({}), 10000.0);
    expect_amount(funded(0.0).totalValue({}), 0.0);
    EXPECT_FALSE(Portfolio::create(PortfolioId::create(1).value(), Money::create(-1.0).value()));
}

TEST(PortfolioTest, BuysDebitCashAndRecordExecutedTransactions) {
    auto portfolio = funded();
    const auto spy = fill(1, "SPY", OrderSide::Buy, 10.0, 100.0);
    const auto apple = fill(2, "AAPL", OrderSide::Buy, 20.0, 50.0, 1);
    ASSERT_TRUE(portfolio.applyTrade(spy));
    ASSERT_TRUE(portfolio.applyTrade(apple));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 8000.0);
    EXPECT_DOUBLE_EQ(portfolio.startingCash().value(), 10000.0);
    ASSERT_EQ(portfolio.positions().size(), 2U);
    EXPECT_EQ(portfolio.positions()[0].symbol(), spy.symbol());
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].quantity().value(), 10.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[1].quantity().value(), 20.0);
    EXPECT_EQ(portfolio.transactionHistory(),
              (std::vector<Transaction>{record(spy), record(apple)}));
    expect_amount(portfolio.marketValue({mark("SPY", 100.0), mark("AAPL", 50.0)}), 2000.0);
    expect_amount(portfolio.totalValue({mark("SPY", 100.0), mark("AAPL", 50.0)}), 10000.0);
}

TEST(PortfolioTest, SalesCreditCashAndTotalReconcilesWithPositionProfit) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 10.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "AAPL", OrderSide::Buy, 20.0, 50.0, 1)));
    ASSERT_TRUE(portfolio.applyTrade(fill(3, "SPY", OrderSide::Sell, 4.0, 120.0, 2)));
    const std::vector<MarketPrice> marks{mark("SPY", 120.0), mark("AAPL", 45.0)};
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 8480.0);
    expect_amount(portfolio.marketValue(marks), 1620.0);
    expect_amount(portfolio.totalValue(marks), 10100.0);

    ASSERT_TRUE(portfolio.applyTrade(fill(4, "AAPL", OrderSide::Sell, 20.0, 45.0, 3)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 9380.0);
    ASSERT_EQ(portfolio.positions().size(), 2U);
    const auto& spy = portfolio.positions()[0];
    const auto& apple = portfolio.positions()[1];
    EXPECT_DOUBLE_EQ(spy.quantity().value(), 6.0);
    EXPECT_DOUBLE_EQ(apple.quantity().value(), 0.0);
    EXPECT_DOUBLE_EQ(spy.realized_pnl().value() + apple.realized_pnl().value(), -20.0);
    expect_amount(spy.unrealized_pnl(Price::create(120.0).value()), 120.0);
    expect_amount(portfolio.marketValue({mark("SPY", 120.0)}), 720.0);
    expect_amount(portfolio.totalValue({mark("SPY", 120.0)}), 10100.0);
    // Total - starting cash = realized + unrealized = -20 + 120 = 100.
}

TEST(PortfolioTest, ExactCashPurchaseAndLiquidationPreserveValueAtFillMark) {
    auto portfolio = funded(100.0);
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 0.5, 200.0)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 0.0);
    expect_amount(portfolio.totalValue({mark("SPY", 200.0)}), 100.0);
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "SPY", OrderSide::Sell, 0.5, 200.0, 1)));
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 100.0);
    expect_amount(portfolio.marketValue({}), 0.0);
    expect_amount(portfolio.totalValue({}), 100.0);
    ASSERT_EQ(portfolio.positions().size(), 1U);
    EXPECT_FALSE(portfolio.positions()[0].average_cost());
}

TEST(PortfolioTest, AdditionalBuysAndReopeningReuseThePosition) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 10.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "SPY", OrderSide::Buy, 10.0, 120.0, 1)));
    ASSERT_EQ(portfolio.positions().size(), 1U);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].average_cost()->value(), 110.0);
    ASSERT_TRUE(portfolio.applyTrade(fill(3, "SPY", OrderSide::Sell, 20.0, 120.0, 2)));
    ASSERT_TRUE(portfolio.applyTrade(fill(4, "SPY", OrderSide::Buy, 1.0, 50.0, 3)));
    ASSERT_EQ(portfolio.positions().size(), 1U);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].average_cost()->value(), 50.0);
    EXPECT_DOUBLE_EQ(portfolio.positions()[0].realized_pnl().value(), 200.0);
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 10150.0);
}

TEST(PortfolioTest, InsufficientCashAndTinyOverspendLeaveEverythingUnchanged) {
    auto portfolio = funded(100.0);
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 2.0, 100.0)));
    EXPECT_FALSE(
        portfolio.applyTrade(fill(2, "SPY", OrderSide::Buy, 1.0, std::nextafter(100.0, 200.0))));
    EXPECT_EQ(portfolio.snapshot(), before);
    // Rejections neither consume the OrderId nor append history.
    EXPECT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
}

TEST(PortfolioTest, SellsCannotCreateShortPositionsOrCreditRejectedProceeds) {
    auto portfolio = funded();
    EXPECT_FALSE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Sell, 1.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTrade(fill(2, "SPY", OrderSide::Sell, 2.0, 100.0, 1)));
    EXPECT_FALSE(
        portfolio.applyTrade(fill(3, "SPY", OrderSide::Sell, std::nextafter(1.0, 2.0), 100.0, 1)));
    EXPECT_FALSE(portfolio.applyTrade(fill(4, "AAPL", OrderSide::Sell, 1.0, 100.0, 1)));
    EXPECT_EQ(portfolio.snapshot(), before);
}

TEST(PortfolioTest, RejectsDuplicateOrderIdsIncludingChangedPayloadAndOtherSymbols) {
    auto portfolio = funded();
    const auto trade = fill(7, "SPY", OrderSide::Buy, 1.0, 100.0);
    ASSERT_TRUE(portfolio.applyTrade(trade));
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTrade(trade));
    EXPECT_FALSE(portfolio.applyTrade(fill(7, "SPY", OrderSide::Buy, 2.0, 101.0, 1)));
    EXPECT_FALSE(portfolio.applyTrade(fill(7, "AAPL", OrderSide::Buy, 1.0, 50.0, 1)));
    EXPECT_FALSE(portfolio.applyTrade(fill(7, "SPY", OrderSide::Sell, 1.0, 100.0, 1)));
    EXPECT_EQ(portfolio.snapshot(), before);
}

TEST(PortfolioTest, EnforcesGlobalChronologyButPreservesEqualTimeOrder) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0, 10)));
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTrade(fill(2, "AAPL", OrderSide::Buy, 1.0, 50.0, 9)));
    EXPECT_EQ(portfolio.snapshot(), before);
    const auto same_time = fill(2, "AAPL", OrderSide::Buy, 1.0, 50.0, 10);
    ASSERT_TRUE(portfolio.applyTrade(same_time));
    ASSERT_EQ(portfolio.transactionHistory().size(), 2U);
    EXPECT_EQ(portfolio.transactionHistory()[1], record(same_time));
}

TEST(PortfolioTest, ReplayReconstructsCashPositionsAndHistoryExactly) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 10.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "AAPL", OrderSide::Buy, 20.0, 50.0, 1)));
    ASSERT_TRUE(portfolio.applyTrade(fill(3, "SPY", OrderSide::Sell, 4.0, 120.0, 2)));
    ASSERT_TRUE(portfolio.applyTrade(fill(4, "AAPL", OrderSide::Sell, 20.0, 45.0, 3)));
    const auto replayed =
        Portfolio::replay(portfolio.id(), portfolio.startingCash(), portfolio.transactionHistory());
    ASSERT_TRUE(replayed);
    EXPECT_EQ(replayed->snapshot(), portfolio.snapshot());
    const auto empty_replay = Portfolio::replay(portfolio.id(), portfolio.startingCash(), {});
    ASSERT_TRUE(empty_replay);
    EXPECT_EQ(empty_replay->snapshot(), funded().snapshot());
}

TEST(PortfolioTest, ReplayRejectsInconsistentHistoryWithoutRepair) {
    const auto valid = record(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0, 1));
    const auto id = PortfolioId::create(1).value();
    const auto cash = Money::create(10000.0).value();
    EXPECT_FALSE(Portfolio::replay(id, cash, {valid, valid}));
    EXPECT_FALSE(Portfolio::replay(id, cash,
                                   {valid, record(fill(2, "AAPL", OrderSide::Buy, 1.0, 50.0, 0))}));
    EXPECT_FALSE(Portfolio::replay(
        id, cash, {valid, record(fill(2, "SPY", OrderSide::Sell, 2.0, 100.0, 2))}));
    EXPECT_FALSE(Portfolio::replay(id, Money::create(0.0).value(), {valid}));
    EXPECT_FALSE(Portfolio::replay(id, Money::create(-1.0).value(), {}));
}

TEST(PortfolioTest, RequiresEveryOpenMarkWithoutGuessing) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "AAPL", OrderSide::Buy, 1.0, 50.0, 1)));
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.marketValue({}));
    EXPECT_FALSE(portfolio.marketValue({mark("SPY", 100.0)}));
    EXPECT_FALSE(portfolio.totalValue({mark("AAPL", 50.0)}));
    EXPECT_FALSE(portfolio.marketValue({mark("spy", 100.0), mark("AAPL", 50.0)}));
    expect_amount(portfolio.marketValue({mark("AAPL", 60.0), mark("SPY", 110.0)}), 170.0);
    expect_amount(
        portfolio.marketValue({mark("SPY", 110.0), mark("AAPL", 60.0), mark("MSFT", 500.0)}),
        170.0);
    EXPECT_EQ(portfolio.snapshot(), before);
}

TEST(PortfolioTest, RejectsDuplicateMarksEvenIfIdenticalOrUnused) {
    auto portfolio = funded();
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
    EXPECT_FALSE(portfolio.marketValue({mark("SPY", 100.0), mark("SPY", 100.0)}));
    EXPECT_FALSE(portfolio.totalValue({mark("SPY", 100.0), mark("SPY", 101.0)}));
    EXPECT_FALSE(
        portfolio.marketValue({mark("SPY", 100.0), mark("AAPL", 50.0), mark("AAPL", 60.0)}));
}

// Stand-in for the future strategy signature. It only proposes an Order; trusted
// orchestration must separately execute and apply an accepted Trade.
std::optional<Order> propose_buy(const PortfolioSnapshot& input) {
    if (input.cashBalance().value() < 100.0) {
        return std::nullopt;
    }
    return Order::create_market(OrderId::create(99).value(), Symbol::create("SPY").value(),
                                OrderSide::Buy, Quantity::create(1.0).value(),
                                Timestamp{Timestamp::Value{}});
}

TEST(PortfolioBoundaryTest, StrategyReadsSnapshotAndProposesWithoutMutation) {
    auto portfolio = funded();
    const auto input = portfolio.snapshot();
    const auto proposal = propose_buy(input);
    ASSERT_TRUE(proposal);
    EXPECT_EQ(portfolio.snapshot(), input);
    EXPECT_TRUE(portfolio.transactionHistory().empty());
    const auto trade =
        Trade::create(*proposal, Price::create(100.0).value(), proposal->timestamp());
    ASSERT_TRUE(trade);
    ASSERT_TRUE(portfolio.applyTrade(*trade));
    EXPECT_DOUBLE_EQ(input.cashBalance().value(), 10000.0);
    EXPECT_TRUE(input.positions().empty());
    EXPECT_DOUBLE_EQ(portfolio.cashBalance().value(), 9900.0);
}

TEST(PortfolioBoundaryTest, SnapshotOwnsItsDataAfterAuthorityChangesOrDies) {
    const auto input = [] {
        auto portfolio = funded();
        EXPECT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
        return portfolio.snapshot();
    }();
    ASSERT_EQ(input.positions().size(), 1U);
    ASSERT_EQ(input.transactionHistory().size(), 1U);
    EXPECT_DOUBLE_EQ(input.cashBalance().value(), 9900.0);
    expect_amount(input.marketValue({mark("SPY", 120.0)}), 120.0);
    expect_amount(input.totalValue({mark("SPY", 120.0)}), 10020.0);
    auto local_positions = input.positions();
    auto local_history = input.transactionHistory();
    local_positions.clear();
    local_history.clear();
    EXPECT_EQ(input.positions().size(), 1U);
    EXPECT_EQ(input.transactionHistory().size(), 1U);
}

TEST(PortfolioBoundaryTest, CopyMoveAndAssignmentKeepCoherentIndependentState) {
    auto original = funded();
    ASSERT_TRUE(original.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 100.0)));
    const auto expected = original.snapshot();
    auto moved = std::move(original);
    EXPECT_EQ(original.snapshot(), expected);
    EXPECT_EQ(moved.snapshot(), expected);
    auto snapshot = original.snapshot();
    const auto moved_snapshot = std::move(snapshot);
    EXPECT_EQ(snapshot, expected);
    EXPECT_EQ(moved_snapshot, expected);
    auto assigned = funded(0.0);
    assigned = moved;
    ASSERT_TRUE(assigned.applyTrade(fill(2, "SPY", OrderSide::Sell, 1.0, 120.0, 1)));
    EXPECT_EQ(moved.snapshot(), expected);
    EXPECT_DOUBLE_EQ(assigned.cashBalance().value(), 10020.0);
}

TEST(PortfolioNumericTest, RejectsTradeNotionalOverflowAndUnderflowAtomically) {
    auto portfolio = funded(std::numeric_limits<double>::max());
    const auto before = portfolio.snapshot();
    EXPECT_FALSE(portfolio.applyTrade(
        fill(1, "SPY", OrderSide::Buy, std::numeric_limits<double>::max(), 2.0)));
    EXPECT_FALSE(
        portfolio.applyTrade(fill(2, "SPY", OrderSide::Buy, std::numeric_limits<double>::min(),
                                  std::numeric_limits<double>::min())));
    EXPECT_EQ(portfolio.snapshot(), before);
}

TEST(PortfolioNumericTest, RejectsCashOverflowAndSwallowedDebit) {
    auto large = funded(std::numeric_limits<double>::max());
    ASSERT_TRUE(large.applyTrade(
        fill(1, "SPY", OrderSide::Buy, 1.0, std::numeric_limits<double>::max() / 4.0)));
    const auto before = large.snapshot();
    EXPECT_FALSE(large.applyTrade(
        fill(2, "SPY", OrderSide::Sell, 1.0, std::numeric_limits<double>::max() / 2.0, 1)));
    EXPECT_EQ(large.snapshot(), before);
    auto cash = funded(1e20);
    EXPECT_FALSE(cash.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 1.0)));
    EXPECT_EQ(cash.snapshot(), funded(1e20).snapshot());
}

TEST(PortfolioNumericTest, PositionArithmeticFailureDoesNotDebitCashOrAppendHistory) {
    auto portfolio = funded(1e22);
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1e20, 1.0)));
    const auto before = portfolio.snapshot();
    // The debit is representable, but adding one share to 1e20 is not.
    EXPECT_FALSE(portfolio.applyTrade(fill(2, "SPY", OrderSide::Buy, 1.0, 1e10, 1)));
    EXPECT_EQ(portfolio.snapshot(), before);
}

TEST(PortfolioNumericTest, RejectsSaleCreditsThatLoseEitherCashContribution) {
    auto large_cash = funded(1e20);
    ASSERT_TRUE(large_cash.applyTrade(fill(1, "SPY", OrderSide::Buy, 1e10, 1.0)));
    const auto before = large_cash.snapshot();
    // The share reduction is representable, but a $1 cash credit would disappear.
    EXPECT_FALSE(large_cash.applyTrade(fill(2, "SPY", OrderSide::Sell, 1.0, 1.0, 1)));
    EXPECT_EQ(large_cash.snapshot(), before);

    auto small_cash = funded(1.0);
    ASSERT_TRUE(small_cash.applyTrade(fill(1, "SPY", OrderSide::Buy, 0.5, 1.0)));
    const auto small_before = small_cash.snapshot();
    // A huge credit must not silently discard the pre-existing $0.50 either.
    EXPECT_FALSE(small_cash.applyTrade(fill(2, "SPY", OrderSide::Sell, 0.5, 1e20, 1)));
    EXPECT_EQ(small_cash.snapshot(), small_before);
}

TEST(PortfolioNumericTest, RejectsOverflowOrLostHoldingsDuringValuation) {
    auto portfolio = funded(100.0);
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 1.0)));
    ASSERT_TRUE(portfolio.applyTrade(fill(2, "AAPL", OrderSide::Buy, 1.0, 1.0, 1)));
    const double maximum = std::numeric_limits<double>::max();
    EXPECT_FALSE(portfolio.marketValue({mark("SPY", maximum), mark("AAPL", maximum)}));
    EXPECT_FALSE(portfolio.marketValue({mark("SPY", 1e20), mark("AAPL", 1.0)}));
    EXPECT_FALSE(portfolio.marketValue({mark("SPY", 1.0), mark("AAPL", 1e20)}));
}

TEST(PortfolioNumericTest, RejectsTotalValueThatWouldLoseCash) {
    auto portfolio = funded(2.0);
    ASSERT_TRUE(portfolio.applyTrade(fill(1, "SPY", OrderSide::Buy, 1.0, 1.0)));
    expect_amount(portfolio.marketValue({mark("SPY", 1e20)}), 1e20);
    EXPECT_FALSE(portfolio.totalValue({mark("SPY", 1e20)}));
}

}  // namespace
