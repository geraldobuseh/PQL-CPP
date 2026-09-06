#include <gtest/gtest.h>

#include <future>
#include <pqxx/pqxx>

#include "alpha_fixture.hpp"
#include "market_data/validation.hpp"
#include "persistence/postgres.hpp"

namespace {
using namespace pql;
using namespace pql::persistence;
const auto today = *Date::create(2026, 1, 6);
std::vector<PriceBar> bars(const Symbol& symbol) {
    return parseAlphaVantageDaily(alphaFixture(symbol.value()).dump(), symbol, today);
}

TEST(DailyIngestion, AllEightSymbolsStoreOnceAndRetryWithoutDuplicates) {
    for (const auto& symbol : supportedMarketSymbols()) {
        FixtureTransport transport;
        transport.responses = {{200, alphaFixture(symbol.value()).dump(), {}}};
        AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
        const auto history =
            provider.getHistory(symbol, *Date::create(2026, 1, 2), *Date::create(2026, 1, 5));
        {
            PostgresUnitOfWork work;
            const auto counts = work.storeDailyBars(symbol, "alphavantage.daily", history);
            EXPECT_EQ(counts.inserted, 2U);
            EXPECT_EQ(counts.unchanged, 0U);
            work.commit();
        }
        {
            PostgresUnitOfWork work;
            const auto counts = work.storeDailyBars(symbol, "alphavantage.daily", history);
            EXPECT_EQ(counts.inserted, 0U);
            EXPECT_EQ(counts.unchanged, 2U);
            work.commit();
        }
    }
    pqxx::connection connection;
    pqxx::work tx(connection);
    EXPECT_EQ(tx.exec("SELECT count(*) FROM market_prices WHERE source='alphavantage.daily'")[0][0]
                  .as<int>(),
              16);
    EXPECT_EQ(
        tx.exec(
              "SELECT count(*) FROM market_prices WHERE source='alphavantage.daily' AND "
              "session_date IS NOT NULL AND observed_at=session_date::timestamp AT TIME ZONE 'UTC'")
            [0][0]
                .as<int>(),
        16);
}
TEST(DailyIngestion, ConflictingRevisionRollsBackEarlierBatchInsert) {
    const auto symbol = *Symbol::create("AAPL");
    auto original = bars(symbol);
    {
        PostgresUnitOfWork work;
        (void)work.storeDailyBars(symbol, "revision.test", {original[1]});
        work.commit();
    }
    auto changed = alphaFixture();
    changed["Time Series (Daily)"]["2026-01-05"]["4. close"] = "116.0000";
    const auto conflict = parseAlphaVantageDaily(changed.dump(), symbol, today);
    {
        PostgresUnitOfWork work;
        EXPECT_THROW((void)work.storeDailyBars(symbol, "revision.test", conflict),
                     PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    pqxx::connection connection;
    pqxx::work tx(connection);
    EXPECT_EQ(
        tx.exec("SELECT count(*) FROM market_prices WHERE source='revision.test'")[0][0].as<int>(),
        1);
    EXPECT_EQ(
        tx.exec("SELECT close FROM market_prices WHERE source='revision.test'")[0][0].as<int>(),
        115);
}
TEST(DailyIngestion, ConcurrentIdenticalBatchesCommitOneCopy) {
    const auto symbol = *Symbol::create("MSFT");
    const auto data = bars(symbol);
    PostgresUnitOfWork first;
    EXPECT_EQ(first.storeDailyBars(symbol, "concurrent.test", data).inserted, 2U);
    auto future = std::async(std::launch::async, [&] {
        PostgresUnitOfWork second;
        const auto result = second.storeDailyBars(symbol, "concurrent.test", data);
        second.commit();
        return result;
    });
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    first.commit();
    EXPECT_EQ(future.get().unchanged, 2U);
}
TEST(DailyIngestion, InvalidBatchesCannotCommitEarlierChanges) {
    const auto symbol = *Symbol::create("NVDA");
    const auto ordered = bars(symbol);
    const std::vector<std::vector<PriceBar>> invalid{
        {ordered[0], ordered[0]}, {ordered[1], ordered[0]}, bars(*Symbol::create("META"))};
    for (const auto& data : invalid) {
        PostgresUnitOfWork work;
        EXPECT_THROW((void)work.storeDailyBars(symbol, "invalid.test", data), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    pqxx::connection connection;
    pqxx::work tx(connection);
    EXPECT_EQ(
        tx.exec("SELECT count(*) FROM market_prices WHERE source='invalid.test'")[0][0].as<int>(),
        0);
}
TEST(DailyIngestion, InvalidProviderResponsePersistsNoGoodPrefix) {
    const auto symbol = *Symbol::create("AAPL");
    auto data = alphaFixture();
    data["Time Series (Daily)"]["2026-01-05"]["2. high"] = "90";
    FixtureTransport transport;
    transport.responses = {{200, data.dump(), {}}};
    AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
    bool reached_storage = false;
    try {
        const auto history =
            provider.getHistory(symbol, *Date::create(2026, 1, 2), *Date::create(2026, 1, 5));
        reached_storage = true;
        PostgresUnitOfWork work;
        (void)work.storeDailyBars(symbol, "validation.test", history);
        work.commit();
        FAIL() << "Expected rejection before storage";
    } catch (const MarketDataValidationError& error) {
        EXPECT_EQ(error.issue(), MarketDataIssue::InvalidOhlc);
    }
    EXPECT_FALSE(reached_storage);
    pqxx::connection connection;
    pqxx::work tx(connection);
    EXPECT_EQ(tx.exec("SELECT count(*) FROM market_prices WHERE source='validation.test'")[0][0]
                  .as<int>(),
              0);
}
}  // namespace
