#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <limits>
#include <pqxx/pqxx>

#include "persistence/postgres.hpp"

namespace {
using namespace pql;
using namespace pql::persistence;
Symbol symbol(const char* text) { return *Symbol::create(text); }
Timestamp at(std::int64_t ms) { return Timestamp{Timestamp::Value{std::chrono::milliseconds{ms}}}; }
Money money(double value) { return *Money::create(value); }
Price quote(double value) { return *Price::create(value); }
Quantity quantity(double value) { return *Quantity::create(value); }
Order order(std::int64_t id, const Symbol& asset, OrderSide side, double size,
            std::int64_t ms = 0) {
    return *Order::create_market(*OrderId::create(id), asset, side, quantity(size), at(ms));
}
Transaction event(PortfolioId id, const Order& request, double price, double fee = 0,
                  std::int64_t ms = 0) {
    return *Transaction::create(id, *Trade::create(request, quote(price), at(ms)), money(fee));
}
PortfolioId seed(const Symbol& asset, double cash = 1000) {
    PostgresUnitOfWork work;
    work.addAsset(asset, "Synthetic asset");
    auto id = work.createPortfolio("Original", PortfolioKind::Simulated, money(cash));
    work.commit();
    return id;
}

TEST(Postgres, InsertReadRenameAndFeeAwareReplayAcrossConnections) {
    const auto asset = symbol("LEDGER");
    const auto id = seed(asset);
    const auto buy = order(1, asset, OrderSide::Buy, 2, -1001);
    const auto sell = order(2, asset, OrderSide::Sell, 1, -1000);
    const auto first = event(id, buy, 100, 2, -999);
    const auto second = event(id, sell, 120, 1, -999);
    {
        PostgresUnitOfWork work;
        work.append(buy, first);
        work.append(sell, second);
        work.renamePortfolio(id, "Investor's portfolio; SELECT 'safe'");
        work.commit();
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    PostgresUnitOfWork read;
    const auto loaded = read.portfolio(id);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded->name, "Investor's portfolio; SELECT 'safe'");
    EXPECT_EQ(loaded->kind, PortfolioKind::Simulated);
    EXPECT_DOUBLE_EQ(loaded->state.cashBalance().value(), 917);
    ASSERT_EQ(loaded->state.positions().size(), 1U);
    EXPECT_DOUBLE_EQ(loaded->state.positions()[0].quantity().value(), 1);
    EXPECT_EQ(read.transactions(id), (std::vector<Transaction>{first, second}));
    EXPECT_DOUBLE_EQ(loaded->state.totalValue({{asset, quote(120)}})->value(), 1037);
    EXPECT_DOUBLE_EQ(loaded->state.positions()[0].realized_pnl().value(), 18);
}

TEST(Postgres, PriceRoundTripAndMissingLookups) {
    const auto asset = symbol("QUOTES");
    seed(asset);
    const PriceObservation bar{asset,           at(-86400001), "provider's feed",
                               Adjustment::Raw, quote(0.1),    quote(0.3),
                               quote(0.1),      quote(0.2),    quantity(0.125)};
    {
        PostgresUnitOfWork work;
        work.insertPrice(bar);
        work.commit();
    }
    PostgresUnitOfWork read;
    EXPECT_EQ(read.price(asset, bar.timestamp, bar.source, bar.adjustment), bar);
    EXPECT_FALSE(read.price(asset, at(0), bar.source, bar.adjustment));
    EXPECT_FALSE(read.portfolio(*PortfolioId::create(std::numeric_limits<std::int64_t>::max())));
}

TEST(Postgres, DestructorRollsBackAcrossRepositories) {
    const auto asset = symbol("ABANDONED");
    const auto id = seed(asset);
    const auto request = order(1, asset, OrderSide::Buy, 1);
    {
        PostgresUnitOfWork work;
        work.renamePortfolio(id, "Not committed");
        work.append(request, event(id, request, 100));
    }
    PostgresUnitOfWork read;
    EXPECT_EQ(read.portfolio(id)->name, "Original");
    EXPECT_TRUE(read.transactions(id).empty());
}

TEST(Postgres, SqlFailureRollsBackEarlierWritesAndPoisonsCommit) {
    const auto asset = symbol("SQLFAIL");
    const auto id = seed(asset);
    const auto request = order(1, asset, OrderSide::Buy, 1);
    {
        PostgresUnitOfWork work;
        work.append(request, event(id, request, 100));
        work.renamePortfolio(id, "Rolled back");
        EXPECT_THROW(work.addAsset(asset, "Duplicate"), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    PostgresUnitOfWork read;
    EXPECT_TRUE(read.transactions(id).empty());
    EXPECT_EQ(read.portfolio(id)->name, "Original");
    pqxx::connection connection;
    pqxx::work check(connection);
    EXPECT_EQ(check
                  .exec("SELECT count(*) FROM orders WHERE portfolio_id=$1",
                        pqxx::params{id.value()})[0][0]
                  .as<int>(),
              0);
    EXPECT_EQ(check
                  .exec("SELECT count(*) FROM executions WHERE portfolio_id=$1",
                        pqxx::params{id.value()})[0][0]
                  .as<int>(),
              0);
}

TEST(Postgres, DomainFailureCannotCommitEarlierMetadataChange) {
    const auto asset = symbol("OVERSOLD");
    const auto id = seed(asset);
    const auto sell = order(1, asset, OrderSide::Sell, 1);
    {
        PostgresUnitOfWork work;
        work.renamePortfolio(id, "Rejected");
        EXPECT_THROW(work.append(sell, event(id, sell, 100)), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    PostgresUnitOfWork read;
    EXPECT_EQ(read.portfolio(id)->name, "Original");
    EXPECT_TRUE(read.transactions(id).empty());
}

TEST(Postgres, DuplicateAndMismatchedRequestsAreRejected) {
    const auto asset = symbol("DUPLICATE");
    const auto id = seed(asset);
    const auto request = order(1, asset, OrderSide::Buy, 1);
    {
        PostgresUnitOfWork work;
        work.append(request, event(id, request, 100));
        work.commit();
    }
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.append(request, event(id, request, 100)), PersistenceError);
    }
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.append(order(2, asset, OrderSide::Buy, 1), event(id, request, 100)),
                     PersistenceError);
    }
    PostgresUnitOfWork read;
    EXPECT_EQ(read.transactions(id).size(), 1U);
}

TEST(Postgres, CompetingPurchasesSerializeAndCannotOverspend) {
    const auto asset = symbol("COMPETING");
    const auto id = seed(asset, 100);
    PostgresUnitOfWork first;
    const auto buy1 = order(1, asset, OrderSide::Buy, 1);
    first.append(buy1, event(id, buy1, 80));
    std::promise<void> ready;
    auto signal = ready.get_future();
    auto competing = std::async(std::launch::async, [&] {
        PostgresUnitOfWork second;
        ready.set_value();
        const auto buy2 = order(2, asset, OrderSide::Buy, 1);
        try {
            second.append(buy2, event(id, buy2, 80));
            second.commit();
            return true;
        } catch (const PersistenceError&) {
            return false;
        }
    });
    ASSERT_EQ(signal.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(competing.wait_for(std::chrono::milliseconds(100)), std::future_status::timeout);
    first.commit();
    EXPECT_FALSE(competing.get());
    PostgresUnitOfWork read;
    EXPECT_DOUBLE_EQ(read.portfolio(id)->state.cashBalance().value(), 20);
    EXPECT_EQ(read.transactions(id).size(), 1U);
}

TEST(Postgres, NoncanonicalDatabaseDecimalsFailInsteadOfLosingPrecision) {
    const auto asset = symbol("PRECISION");
    const auto id = seed(asset);
    // Direct SQL is confined to fixtures: represent externally imported data.
    pqxx::connection connection;
    {
        pqxx::work tx(connection);
        tx.exec(
            "INSERT INTO market_prices SELECT asset_id,TIMESTAMPTZ "
            "'epoch','external','raw',0.100000000000000001,1,0.1,1,0 FROM assets WHERE "
            "symbol='PRECISION'");
        tx.commit();
    }
    PostgresUnitOfWork read;
    EXPECT_THROW((void)read.price(asset, at(0), "external", Adjustment::Raw), PersistenceError);
    EXPECT_THROW(read.renamePortfolio(id, "After failure"), PersistenceError);
}

TEST(Postgres, ExtremeFiniteDoublesRoundTrip) {
    const auto asset = symbol("EXTREME");
    seed(asset);
    const auto tiny = std::numeric_limits<double>::denorm_min();
    const auto large = std::numeric_limits<double>::max();
    const PriceObservation bar{asset,           at(1),        "synthetic",
                               Adjustment::Raw, quote(tiny),  quote(large),
                               quote(tiny),     quote(large), quantity(0)};
    {
        PostgresUnitOfWork work;
        work.insertPrice(bar);
        work.commit();
    }
    PostgresUnitOfWork read;
    EXPECT_EQ(read.price(asset, at(1), "synthetic", Adjustment::Raw), bar);
}

TEST(Postgres, InvalidOhlcAndUnsupportedTimestampsRollback) {
    const auto asset = symbol("BADBAR");
    seed(asset);
    const PriceObservation bad{asset,     at(1),     "synthetic", Adjustment::Raw, quote(100),
                               quote(90), quote(80), quote(100),  quantity(0)};
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.insertPrice(bad), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    auto outside = bad;
    outside.high = quote(110);
    outside.timestamp = at(std::numeric_limits<std::int64_t>::max());
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.insertPrice(outside), PersistenceError);
    }
}

TEST(Postgres, EmbeddedNullTextRejectsInsteadOfAliasingAndPoisonsWork) {
    const auto asset = symbol("NULLTEXT");
    const auto id = seed(asset);
    const PriceObservation bar{asset,    at(0),    "feed",   Adjustment::Raw, quote(1),
                               quote(1), quote(1), quote(1), quantity(0)};
    {
        PostgresUnitOfWork work;
        work.insertPrice(bar);
        work.commit();
    }
    const std::string hidden{"feed\0other", 10};
    {
        PostgresUnitOfWork work;
        work.renamePortfolio(id, "Should roll back");
        EXPECT_THROW((void)work.price(asset, at(0), hidden, Adjustment::Raw), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.renamePortfolio(id, hidden), PersistenceError);
    }
    {
        PostgresUnitOfWork work;
        EXPECT_THROW((void)work.createPortfolio(hidden, PortfolioKind::Real, money(1)),
                     PersistenceError);
    }
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.addAsset(symbol("NULLNAME"), hidden), PersistenceError);
    }
    auto invalid_bar = bar;
    invalid_bar.source = hidden;
    {
        PostgresUnitOfWork work;
        EXPECT_THROW(work.insertPrice(invalid_bar), PersistenceError);
    }
    EXPECT_THROW(PostgresUnitOfWork{hidden}, PersistenceError);
    PostgresUnitOfWork read;
    EXPECT_EQ(read.portfolio(id)->name, "Original");
}

TEST(Postgres, FailureAfterOrderInsertLeavesNoOrphan) {
    const auto asset = symbol("MIDAPPEND");
    const auto id = seed(asset);
    const auto request = order(1, asset, OrderSide::Buy, 1);
    {
        PostgresUnitOfWork work;
        const auto outside = event(id, request, 100, 0, std::numeric_limits<std::int64_t>::max());
        EXPECT_THROW(work.append(request, outside), PersistenceError);
        EXPECT_THROW(work.commit(), PersistenceError);
    }
    pqxx::connection connection;
    pqxx::work check(connection);
    EXPECT_EQ(check
                  .exec("SELECT count(*) FROM orders WHERE portfolio_id=$1",
                        pqxx::params{id.value()})[0][0]
                  .as<int>(),
              0);
    EXPECT_EQ(check
                  .exec("SELECT count(*) FROM executions WHERE portfolio_id=$1",
                        pqxx::params{id.value()})[0][0]
                  .as<int>(),
              0);
    EXPECT_EQ(check
                  .exec("SELECT count(*) FROM transactions WHERE portfolio_id=$1",
                        pqxx::params{id.value()})[0][0]
                  .as<int>(),
              0);
}

TEST(Postgres, ImportedOverflowAndUnderflowFailRead) {
    const auto asset = symbol("RANGE");
    seed(asset);
    pqxx::connection connection;
    {
        pqxx::work tx(connection);
        for (const auto* value : {"1e400", "1e-400", "9007199254740993"}) {
            tx.exec(
                "INSERT INTO market_prices SELECT asset_id,TIMESTAMPTZ "
                "'epoch',$1::text,'raw',$1::text::numeric,$1::text::numeric,$1::text::numeric,$1::"
                "text::numeric,0 FROM assets WHERE symbol='RANGE'",
                pqxx::params{value});
        }
        tx.commit();
    }
    for (const auto* value : {"1e400", "1e-400", "9007199254740993"}) {
        PostgresUnitOfWork read;
        EXPECT_THROW((void)read.price(asset, at(0), value, Adjustment::Raw), PersistenceError);
    }
}
}  // namespace
