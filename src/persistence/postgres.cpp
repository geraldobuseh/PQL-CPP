#include "persistence/postgres.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <pqxx/pqxx>

namespace pql::persistence {
namespace {
[[noreturn]] void invalid() { throw PersistenceError("Invalid or inconsistent persistence data"); }

const std::string& textValue(const std::string& value) {
    if (value.find('\0') != std::string::npos) invalid();
    return value;
}

std::string decimal(double value) {
    if (!std::isfinite(value)) invalid();
    if (value == 0) return "0";  // Signed zero has no financial distinction.
    std::array<char, 64> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{}) invalid();
    return {buffer.data(), result.ptr};
}

double amount(pqxx::work& tx, const pqxx::field& field) {
    const auto input = field.as<std::string>();
    double value{};
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || !std::isfinite(value)) invalid();
    // Accept only the canonical decimal represented by this engine value. The
    // database compares numerically so harmless trailing zeroes are accepted.
    if (!tx.exec("SELECT $1::numeric = $2::numeric", pqxx::params{input, decimal(value)})[0][0].as<bool>()) invalid();
    return value;
}

std::int64_t milliseconds(Timestamp time) { return time.value().time_since_epoch().count(); }
Timestamp timestamp(const pqxx::field& field) {
    return Timestamp{Timestamp::Value{std::chrono::milliseconds{field.as<std::int64_t>()}}};
}
const char* sideName(OrderSide side) {
    switch (side) { case OrderSide::Buy: return "Buy"; case OrderSide::Sell: return "Sell"; }
    invalid();
}
const char* kindName(PortfolioKind kind) {
    switch (kind) { case PortfolioKind::Real: return "real"; case PortfolioKind::Simulated: return "simulated"; }
    invalid();
}
const char* adjustmentName(Adjustment adjustment) {
    switch (adjustment) {
        case Adjustment::Raw: return "raw";
        case Adjustment::SplitAdjusted: return "split_adjusted";
        case Adjustment::TotalReturnAdjusted: return "total_return_adjusted";
    }
    invalid();
}
// Used only while handling an exception. Never leak libpq diagnostics (which can
// contain SQL, customer values, or connection credentials) through this boundary.
[[noreturn]] void translate() {
    try { throw; }
    catch (const pqxx::in_doubt_error&) { throw CommitUncertain("Commit outcome unknown; reconcile before retrying"); }
    catch (const pqxx::sql_error& error) { throw PersistenceError("PostgreSQL operation failed (SQLSTATE " + error.sqlstate() + ")"); }
    catch (const pqxx::failure&) { throw PersistenceError("PostgreSQL connection or operation failed"); }
}
}  // namespace

struct PostgresUnitOfWork::Impl {
    pqxx::connection connection;
    pqxx::work tx;
    bool finished{false};
    bool failed{false};

    explicit Impl(const std::string& options) : connection(textValue(options)), tx(connection) {
        tx.exec("SET LOCAL search_path = public, pg_catalog");
        tx.exec("SET LOCAL TIME ZONE 'UTC'");
    }
    void active() const {
        if (failed || finished) throw PersistenceError("Unit of work is no longer usable");
    }
    // Every repository entrypoint shares the failure guard. This template avoids
    // allowing a caught validation exception to accidentally commit prior writes.
    template<class Operation> decltype(auto) run(Operation operation) {
        try { active(); return operation(); }
        catch (...) { failed = true; translate(); }
    }
    pqxx::result lock(PortfolioId id) {
        return tx.exec("SELECT name,kind,starting_cash FROM portfolios WHERE portfolio_id=$1 FOR UPDATE", pqxx::params{id.value()});
    }
    std::int64_t asset(const Symbol& symbol) {
        const auto rows = tx.exec("SELECT asset_id FROM assets WHERE symbol=$1", pqxx::params{symbol.value()});
        if (rows.empty()) throw PersistenceError("Asset not found");
        return rows[0][0].as<std::int64_t>();
    }
    std::vector<Transaction> history(PortfolioId id) {
        const auto rows = tx.exec(R"SQL(
            SELECT t.order_id,t.symbol,t.side,t.quantity,t.price,t.fees,
                   (extract(epoch FROM t.executed_at)*1000)::bigint AS filled_ms,
                   (extract(epoch FROM e.submitted_at)*1000)::bigint AS submitted_ms
            FROM transactions t JOIN executions e USING (portfolio_id,order_id)
            WHERE t.portfolio_id=$1 ORDER BY t.replay_sequence
        )SQL", pqxx::params{id.value()});
        std::vector<Transaction> result;
        result.reserve(rows.size());
        for (const auto& row : rows) {
            const auto order_id = OrderId::create(row[0].as<std::int64_t>());
            const auto symbol = Symbol::create(row[1].as<std::string>());
            const auto direction = row[2].as<std::string>();
            if (direction != "Buy" && direction != "Sell") invalid();
            const auto quantity = Quantity::create(amount(tx,row[3]));
            const auto price = Price::create(amount(tx,row[4]));
            const auto fees = Money::create(amount(tx,row[5]));
            if (!order_id || !symbol || !quantity || !price || !fees) invalid();
            const auto order = Order::create_market(*order_id,*symbol,direction=="Buy" ? OrderSide::Buy : OrderSide::Sell,*quantity,timestamp(row[7]));
            if (!order) invalid();
            const auto trade = Trade::create(*order,*price,timestamp(row[6]));
            if (!trade) invalid();
            const auto event = Transaction::create(id,*trade,*fees);
            if (!event) invalid();
            result.push_back(*event);
        }
        return result;
    }
    Portfolio replay(PortfolioId id, const pqxx::row& seed) {
        const auto cash = Money::create(amount(tx,seed[2]));
        if (!cash) invalid();
        auto result = Portfolio::replay(id,*cash,history(id));
        if (!result) invalid();
        return *result;
    }
};

PostgresUnitOfWork::PostgresUnitOfWork(const std::string& connection) try : impl_(std::make_unique<Impl>(connection)) {}
catch (...) { translate(); }
PostgresUnitOfWork::~PostgresUnitOfWork() = default;

void PostgresUnitOfWork::commit() {
    impl_->run([&] { impl_->tx.commit(); impl_->finished = true; });
}

PortfolioId PostgresUnitOfWork::createPortfolio(const std::string& name, PortfolioKind kind, Money cash) {
    return impl_->run([&] {
        const auto row = impl_->tx.exec("INSERT INTO portfolios(name,kind,currency,starting_cash) VALUES($1,$2,'USD',$3) RETURNING portfolio_id",
            pqxx::params{textValue(name),kindName(kind),decimal(cash.value())})[0];
        const auto id = PortfolioId::create(row[0].as<std::int64_t>());
        if (!id) invalid();
        return *id;
    });
}

std::optional<StoredPortfolio> PostgresUnitOfWork::portfolio(PortfolioId id) {
    return impl_->run([&]() -> std::optional<StoredPortfolio> {
        const auto rows = impl_->lock(id);
        if (rows.empty()) return std::nullopt;
        const auto kind = rows[0][1].as<std::string>();
        if (kind != "real" && kind != "simulated") invalid();
        return StoredPortfolio{rows[0][0].as<std::string>(),kind=="real" ? PortfolioKind::Real : PortfolioKind::Simulated,
            impl_->replay(id,rows[0]).snapshot()};
    });
}

void PostgresUnitOfWork::renamePortfolio(PortfolioId id, const std::string& name) {
    impl_->run([&] {
        if (impl_->tx.exec("UPDATE portfolios SET name=$2 WHERE portfolio_id=$1",pqxx::params{id.value(),textValue(name)}).affected_rows()!=1)
            throw PersistenceError("Portfolio not found");
    });
}

std::vector<Transaction> PostgresUnitOfWork::transactions(PortfolioId id) {
    return impl_->run([&] {
        const auto rows = impl_->lock(id);
        if (rows.empty()) throw PersistenceError("Portfolio not found");
        // Validate the entire history, not just independently well-formed rows.
        return std::vector<Transaction>{impl_->replay(id,rows[0]).transactionHistory()};
    });
}

void PostgresUnitOfWork::append(const Order& request, const Transaction& event) {
    impl_->run([&] {
        if (request.id()!=event.order_id() || request.symbol()!=event.symbol() || request.side()!=event.side() ||
            request.quantity()!=event.quantity() || request.timestamp()>event.timestamp()) invalid();
        const auto rows = impl_->lock(event.portfolio_id());
        if (rows.empty()) throw PersistenceError("Portfolio not found");
        auto state = impl_->replay(event.portfolio_id(),rows[0]);
        if (!state.applyTransaction(event)) invalid();
        const auto last = impl_->tx.exec("SELECT coalesce(max(replay_sequence),0) FROM transactions WHERE portfolio_id=$1",
            pqxx::params{event.portfolio_id().value()})[0][0].as<std::int64_t>();
        if (last==std::numeric_limits<std::int64_t>::max()) invalid();
        const auto asset_id = impl_->asset(event.symbol());
        const auto owner = event.portfolio_id().value();
        const auto order = event.order_id().value();
        // Split days/remainder keeps all timestamp arithmetic integral and exact,
        // including negative epoch times; PostgreSQL rejects out-of-range dates.
        impl_->tx.exec(R"SQL(
            INSERT INTO orders(portfolio_id,order_id,asset_id,side,quantity,submitted_at)
            VALUES($1,$2,$3,$4,$5,TIMESTAMPTZ 'epoch' + ($6::bigint/86400000)::integer*INTERVAL '1 day'
                + ($6::bigint%86400000)::integer*INTERVAL '1 millisecond')
        )SQL",pqxx::params{owner,order,asset_id,sideName(event.side()),decimal(event.quantity().value()),milliseconds(request.timestamp())});
        impl_->tx.exec(R"SQL(
            INSERT INTO executions(portfolio_id,order_id,asset_id,side,quantity,submitted_at,price,fees,executed_at)
            SELECT portfolio_id,order_id,asset_id,side,quantity,submitted_at,$3,$4,
                TIMESTAMPTZ 'epoch' + ($5::bigint/86400000)::integer*INTERVAL '1 day'
                + ($5::bigint%86400000)::integer*INTERVAL '1 millisecond'
            FROM orders WHERE portfolio_id=$1 AND order_id=$2
        )SQL",pqxx::params{owner,order,decimal(event.price().value()),decimal(event.fees().value()),milliseconds(event.timestamp())});
        impl_->tx.exec(R"SQL(
            INSERT INTO transactions(portfolio_id,replay_sequence,order_id,asset_id,symbol,side,quantity,price,fees,executed_at)
            SELECT portfolio_id,$3,order_id,asset_id,$4,side,quantity,price,fees,executed_at
            FROM executions WHERE portfolio_id=$1 AND order_id=$2
        )SQL",pqxx::params{owner,order,last+1,event.symbol().value()});
    });
}

void PostgresUnitOfWork::addAsset(const Symbol& symbol, const std::string& name) {
    impl_->run([&] { impl_->tx.exec("INSERT INTO assets(symbol,name,currency) VALUES($1,$2,'USD')",pqxx::params{symbol.value(),textValue(name)}); });
}

void PostgresUnitOfWork::insertPrice(const PriceObservation& p) {
    impl_->run([&] {
        impl_->tx.exec(R"SQL(
            INSERT INTO market_prices(asset_id,observed_at,source,adjustment,open,high,low,close,volume)
            VALUES($1,TIMESTAMPTZ 'epoch' + ($2::bigint/86400000)::integer*INTERVAL '1 day'
                + ($2::bigint%86400000)::integer*INTERVAL '1 millisecond',$3,$4,$5,$6,$7,$8,$9)
        )SQL",pqxx::params{impl_->asset(p.symbol),milliseconds(p.timestamp),textValue(p.source),adjustmentName(p.adjustment),
            decimal(p.open.value()),decimal(p.high.value()),decimal(p.low.value()),decimal(p.close.value()),decimal(p.volume.value())});
    });
}

std::optional<PriceObservation> PostgresUnitOfWork::price(const Symbol& symbol, Timestamp time, const std::string& source, Adjustment adjustment) {
    return impl_->run([&]() -> std::optional<PriceObservation> {
        const auto rows = impl_->tx.exec(R"SQL(
            SELECT open,high,low,close,volume FROM market_prices p JOIN assets a USING(asset_id)
            WHERE a.symbol=$1 AND observed_at=TIMESTAMPTZ 'epoch' + ($2::bigint/86400000)::integer*INTERVAL '1 day'
                + ($2::bigint%86400000)::integer*INTERVAL '1 millisecond' AND source=$3 AND adjustment=$4
        )SQL",pqxx::params{symbol.value(),milliseconds(time),textValue(source),adjustmentName(adjustment)});
        if (rows.empty()) return std::nullopt;
        const auto& row = rows[0];
        const auto open=Price::create(amount(impl_->tx,row[0]));
        const auto high=Price::create(amount(impl_->tx,row[1]));
        const auto low=Price::create(amount(impl_->tx,row[2]));
        const auto close=Price::create(amount(impl_->tx,row[3]));
        const auto volume=Quantity::create(amount(impl_->tx,row[4]));
        if (!open || !high || !low || !close || !volume) invalid();
        return PriceObservation{symbol,time,source,adjustment,*open,*high,*low,*close,*volume};
    });
}
}  // namespace pql::persistence
