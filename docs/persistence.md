# C++ PostgreSQL persistence (PQL-011)

## Build and run

The ordinary domain build remains independent of PostgreSQL. Enable the optional
`pql_persistence` target with `-DPQL_BUILD_PERSISTENCE=ON`. It requires libpqxx
7.10 or newer (validated with 7.10.0) and libpq. CMake accepts the libpqxx CMake
package, or falls back to pkg-config. Windows package installations can supply
their usual CMake toolchain file or `CMAKE_PREFIX_PATH`.

The reproducible local test path uses a disposable Linux compiler container:

```powershell
docker compose -f compose.yaml -f compose.test.yaml build persistence_tests
docker compose -f compose.yaml -f compose.test.yaml run --rm persistence_tests
```

The existing `.env` supplies the PostgreSQL password without putting it in command
arguments. The runner creates `pql_integration_test`, migrates it, compiles all
targets, runs CTest and drops that database on exit. It refuses to reuse an existing
database. The application database is not a test fixture. If a forced container
termination prevents cleanup, inspect the leftover test database before manually
removing it. Only one test runner can use this fixed test database at a time.

Native integration builds additionally enable `-DPQL_BUILD_POSTGRES_TESTS=ON`.
Provide an isolated migrated database through libpq's PGHOST, PGPORT, PGUSER,
PGPASSWORD and PGDATABASE variables; tests intentionally fail if connectivity or
schema is missing. They create committed fixtures and expect a fresh database.
Use `ctest --test-dir build --output-on-failure` after configuring and building.
Native Windows persistence linking is not validated by the container test path.

## Repository boundary

`repositories.hpp` contains three interfaces and typed DTOs, with no SQL or pqxx
types. `PostgresUnitOfWork` implements these interfaces using one connection and
one `pqxx::work`. All production SQL is in `postgres.cpp`; direct SQL in integration
tests is limited to fixtures and independent database assertions. The implementation
uses [libpqxx parameter binding](https://libpqxx.readthedocs.io/7.10.5/parameters.html).

Trusted execution/orchestration creates a unit, performs repository calls, then
explicitly calls `commit()`. Destruction rolls back uncommitted changes. Any failed
repository operation poisons the unit, so catching an error cannot accidentally
commit earlier writes. A unit cannot be reused after commit or failure, copied or
shared between threads. Repository references cannot outlive their owning unit.
Strategies receive detached `PortfolioSnapshot` values, never repository authority.

```cpp
pql::persistence::PostgresUnitOfWork work; // reads PG* environment
pql::persistence::PortfolioRepository& portfolios = work;
auto id = portfolios.createPortfolio("Research", pql::persistence::PortfolioKind::Simulated,
                                     *pql::Money::create(1000));
portfolios.renamePortfolio(id, "Research portfolio");
work.commit();
```

Portfolio creation accepts nonnegative starting cash. Renaming is the legitimate
update operation; financial events and initial capital cannot be edited. Missing
portfolio/quote lookups return `nullopt`; missing required append owners/assets,
invalid data and database failures throw `PersistenceError`. SQLSTATE is retained
for SQL failures, but SQL text, parameter values and connection diagnostics are
not exposed. `CommitUncertain` means reconcile durable state before retrying; there
is no automatic retry. Runtime logging/metrics and production role provisioning
belong to application composition, not this library.

Asset registration and price insertion are explicit. Quotes are keyed by symbol,
timestamp, source and adjustment. Duplicate inserts reject; OHLC ordering and
nonnegative volume are checked by PostgreSQL. Market data is insert-only in this
ticket. Revision provenance and an explicit correction workflow are deferred.
Names and sources reject embedded NUL bytes before binding to avoid text truncation.

## Financial writes and reads

Append accepts both the original `Order` and its `Transaction`: the transaction's
trade does not retain the submission instant. These must agree on ID, symbol,
direction and full quantity, and execution must not precede submission. This v0.1
path records manual/unattributed simulated full fills; strategy-run attribution,
prepersisted pending orders and partial fills are not added here.

Append locks the portfolio row, loads seed cash and history in replay-sequence
order, calls the existing domain replay and apply logic, allocates the next sequence,
then inserts order, execution and transaction in the same unit. Fees, duplicate
orders, chronology, cash sufficiency and no-short rules therefore share the domain
implementation. All cooperating writers use this lock; direct SQL can bypass
application rules and must be restricted by deployment permissions.

Portfolio reads also lock the owner row and replay history. They never trust cached
positions or snapshots. Locks last until commit/destruction: keep units short and
acquire multiple portfolio locks in a consistent ID order to avoid deadlocks.
Use deployment connection/statement/lock timeouts; the test runner sets bounded
timeouts. Price reads under READ COMMITTED do not promise a historical market-data
snapshot across multiple queries; repeatable backtest data loading is separate.

## Numeric and clock contract

Writes use locale-independent shortest round-trip decimal text for each finite
engine double. Reads parse the entire NUMERIC text, reject range/nonfinite errors,
then compare its reserialized canonical decimal with the original database numeric.
Trailing decimal zeros are harmless; excess precision that changes the value is an
error. For example, `0.1` is supported, but `0.100000000000000001` is rejected rather
than silently becoming `0.1`. This does not make binary floating-point calculations
exact decimal accounting. Signed zero is normalized to zero. Very small and very
large representable finite doubles are covered by integration tests.

IDs and sequence values are checked 64-bit integers. Timestamps are stored with
day plus millisecond-remainder arithmetic relative to UTC epoch; reads extract
integer milliseconds, avoiding floating epoch seconds. PostgreSQL rejects dates
outside its supported calendar range, even if the C++ clock can represent them.
Original request and fill instants are both persisted. No schema migration is
needed: this adapter uses PQL-010's schema as designed.

## Learning checkpoint

Before: a successful insert can look like a successful financial operation.
The useful concept is the unit of work: several writes represent one indivisible
decision. Personal Quant Lab needs it because an execution without its ledger
event would make replay incomplete. For example, if order insertion succeeds but
execution insertion fails, rollback removes the order too. Remember: commit the
financial consequence as a whole, and derive balances from accepted events.
