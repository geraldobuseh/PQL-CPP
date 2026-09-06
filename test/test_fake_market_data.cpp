#include <gtest/gtest.h>

#include <array>
#include <type_traits>
#include <utility>

#include "domain/portfolio.hpp"
#include "market_data/fake_market_data_provider.hpp"

namespace {
using namespace pql;
static_assert(!std::is_copy_assignable_v<FakeMarketDataProvider>);
static_assert(!std::is_move_assignable_v<FakeMarketDataProvider>);

Symbol symbol(const char* value) { return *Symbol::create(value); }
Date day(unsigned value) { return *Date::create(2026, 1, value); }
Price price(double value) { return *Price::create(value); }
PriceBar bar(const char* ticker, unsigned date, double close) {
    return *PriceBar::create(symbol(ticker), day(date), price(close), price(close),
                             price(close), price(close), *Quantity::create(100));
}

TEST(FakeMarketData, OwnsFixturesAndReturnsIndependentRepeatableResults) {
    std::vector<PriceBar> input{bar("AAPL", 2, 100), bar("AAPL", 5, 120)};
    FakeMarketDataProvider fake(input);
    input.clear();
    MarketDataProvider& provider = fake;
    const FakeMarketDataProvider copy(fake);
    FakeMarketDataProvider from_rvalue(std::move(fake));
    FakeMarketDataProvider another(copy);
    EXPECT_EQ(from_rvalue.getLatestPrice(symbol("AAPL")), price(120));
    EXPECT_EQ(another.getLatestPrice(symbol("AAPL")), price(120));
    EXPECT_EQ(provider.getLatestPrice(symbol("AAPL")), price(120));
    auto result = provider.getHistory(symbol("AAPL"), day(2), day(5));
    ASSERT_EQ(result.size(), 2U);
    result.clear();
    EXPECT_EQ(provider.getHistory(symbol("AAPL"), day(2), day(5)),
              (std::vector<PriceBar>{bar("AAPL", 2, 100), bar("AAPL", 5, 120)}));
    EXPECT_EQ(provider.getHistory(symbol("AAPL"), day(2), day(2)),
              (std::vector<PriceBar>{bar("AAPL", 2, 100)}));
    EXPECT_EQ(provider.getLatestPrice(symbol("AAPL")), price(120));
}

TEST(FakeMarketData, InterleavedSymbolsRemainIndependentAndCaseSensitive) {
    FakeMarketDataProvider fake({bar("AAPL", 2, 100), bar("SPY", 1, 500),
                                 bar("AAPL", 5, 120), bar("SPY", 3, 510)});
    EXPECT_EQ(fake.getLatestPrice(symbol("AAPL")), price(120));
    EXPECT_EQ(fake.getLatestPrice(symbol("SPY")), price(510));
    EXPECT_EQ(fake.getHistory(symbol("SPY"), day(1), day(3)),
              (std::vector<PriceBar>{bar("SPY", 1, 500), bar("SPY", 3, 510)}));
    EXPECT_THROW((void)fake.getLatestPrice(symbol("aapl")), MarketDataError);
}

TEST(FakeMarketData, RejectsDuplicateAndDecreasingDatesRatherThanRepairing) {
    EXPECT_THROW((void)FakeMarketDataProvider({bar("AAPL", 2, 100), bar("AAPL", 2, 100)}), MarketDataError);
    EXPECT_THROW((void)FakeMarketDataProvider({bar("AAPL", 2, 100), bar("SPY", 2, 500), bar("AAPL", 2, 110)}), MarketDataError);
    EXPECT_THROW((void)FakeMarketDataProvider({bar("AAPL", 5, 120), bar("AAPL", 2, 100)}), MarketDataError);
}

TEST(FakeMarketData, DistinguishesUnknownSymbolsEmptyRangesAndInvalidRanges) {
    FakeMarketDataProvider empty({});
    EXPECT_THROW((void)empty.getLatestPrice(symbol("AAPL")), MarketDataError);
    EXPECT_THROW((void)empty.getHistory(symbol("AAPL"), day(1), day(5)), MarketDataError);
    FakeMarketDataProvider fake({bar("AAPL", 2, 100), bar("AAPL", 5, 120)});
    EXPECT_TRUE(fake.getHistory(symbol("AAPL"), day(3), day(4)).empty());
    EXPECT_TRUE(fake.getHistory(symbol("AAPL"), day(6), day(7)).empty());
    EXPECT_THROW((void)fake.getHistory(symbol("UNKNOWN"), day(1), day(5)), MarketDataError);
    EXPECT_THROW((void)fake.getHistory(symbol("UNKNOWN"), day(5), day(1)), std::invalid_argument);
}

TEST(FakeMarketData, EntireFeeAwarePortfolioLifecycleAndReplayAreDeterministic) {
    FakeMarketDataProvider fake({bar("AAPL", 2, 100), bar("AAPL", 5, 120),
                                 bar("AAPL", 6, 130), bar("AAPL", 7, 90)});
    MarketDataProvider& provider = fake;
    const auto asset = symbol("AAPL");
    const auto id = *PortfolioId::create(1);
    const auto capital = *Money::create(1000);
    struct Step {
        unsigned date;
        OrderSide side;
        double quantity;
        double fee;
        double cash;
        double shares;
        double average;
        double realized;
        double value;
        double unrealized;
    };
    const std::array steps{
        Step{2, OrderSide::Buy, 2, 2, 798, 2, 101, 0, 998, -2},
        Step{5, OrderSide::Buy, 2, 2, 556, 4, 111, 0, 1036, 36},
        Step{6, OrderSide::Sell, 1, 1, 685, 3, 111, 18, 1075, 57},
        Step{7, OrderSide::Sell, 3, 3, 952, 0, 0, -48, 952, 0}};
    std::optional<PortfolioSnapshot> first_run;
    for (int run = 0; run < 2; ++run) {
        auto portfolio = Portfolio::create(id, capital);
        ASSERT_TRUE(portfolio);
        std::int64_t sequence = 0;
        for (const auto& step : steps) {
            const auto history = provider.getHistory(asset, day(step.date), day(step.date));
            ASSERT_EQ(history.size(), 1U);
            const auto mark = history.front().close();
            // Synthetic execution steps, not exchange close/availability times.
            // Orders are predetermined, not chosen using future fixture prices.
            const Timestamp time{Timestamp::Value{std::chrono::milliseconds{++sequence}}};
            const auto request = Order::create_market(*OrderId::create(sequence), asset,
                step.side, *Quantity::create(step.quantity), time);
            ASSERT_TRUE(request);
            const auto trade = Trade::create(*request, mark, time);
            ASSERT_TRUE(trade);
            const auto transaction = Transaction::create(id, *trade, *Money::create(step.fee));
            ASSERT_TRUE(transaction);
            ASSERT_TRUE(portfolio->applyTransaction(*transaction));
            const auto snapshot = portfolio->snapshot();
            EXPECT_DOUBLE_EQ(snapshot.cashBalance().value(), step.cash);
            ASSERT_EQ(snapshot.positions().size(), 1U);
            const auto& position = snapshot.positions().front();
            EXPECT_DOUBLE_EQ(position.quantity().value(), step.shares);
            if (step.shares == 0) EXPECT_FALSE(position.average_cost());
            else {
                ASSERT_TRUE(position.average_cost());
                EXPECT_DOUBLE_EQ(position.average_cost()->value(), step.average);
            }
            EXPECT_DOUBLE_EQ(position.realized_pnl().value(), step.realized);
            const auto unrealized = position.unrealized_pnl(mark);
            ASSERT_TRUE(unrealized);
            EXPECT_DOUBLE_EQ(unrealized->value(), step.unrealized);
            const auto total = snapshot.totalValue({{asset, mark}});
            ASSERT_TRUE(total);
            EXPECT_DOUBLE_EQ(total->value(), step.value);
        }
        ASSERT_EQ(portfolio->transactionHistory().size(), 4U);
        const auto replayed = Portfolio::replay(id, capital, portfolio->transactionHistory());
        ASSERT_TRUE(replayed);
        EXPECT_EQ(replayed->snapshot(), portfolio->snapshot());
        if (first_run) EXPECT_EQ(portfolio->snapshot(), *first_run);
        else first_run = portfolio->snapshot();
    }
}
}  // namespace
