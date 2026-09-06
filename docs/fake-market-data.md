# PQL-013: deterministic fake market data

`pql::FakeMarketDataProvider` implements `MarketDataProvider`. Construct it with a
vector of validated `PriceBar` values and link `pql_market_data`. It owns its data;
editing/destroying the input or a returned history vector cannot change the fixture.
There are no credentials, network calls, database access, clock reads or randomness.

Symbols are known when at least one bar is supplied. Dates must increase strictly
within each symbol; interleaved symbols are allowed. Duplicates and descending
dates throw `MarketDataError`, without sorting or repair. History has inclusive
endpoints. Unknown symbols throw, known symbols outside their supplied dates return
empty, and reversed ranges throw `std::invalid_argument`. An empty provider is valid
and has no known symbols. Fixture gaps are intentionally supplied absences; the fake
does not check exchange calendars or simulate incomplete downloads.

Latest price is the final supplied close for that symbol. History queries do not
change it. There is no simulation clock or future-data filter: historical accounting
tests must request the appropriate dates rather than use latest price in their loop.
This is an accounting fixture, not a production backtest implementation.

## Numeric simulation example

Starting cash is $1,000. Orders are predetermined; each fixed historical close is
the synthetic fill and valuation mark, with no additional slippage.

| Operation | Fee | Cash | Shares | Average cost | Cumulative realized P&L | Total value |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Buy 2 at $100 | $2 | $798 | 2 | $101 | $0 | $998 |
| Buy 2 at $120 | $2 | $556 | 4 | $111 | $0 | $1,036 |
| Sell 1 at $130 | $1 | $685 | 3 | $111 | $18 | $1,075 |
| Sell 3 at $90 | $3 | $952 | 0 | absent | -$48 | $952 |

Tests verify each state, unrealized P&L, repeated-run equality and reconstruction
from the transaction ledger. The final loss is $48. This establishes accounting
correctness with known data, not strategy profitability or benchmark performance.

## Run without networking

The existing test image includes the compiler and GoogleTest. Build it once while
dependencies are available:

```powershell
docker compose -f compose.yaml -f compose.test.yaml build persistence_tests
```

Then run from the repository root with networking disabled and persistence off:

```powershell
docker run --rm --network none --mount "type=bind,source=$($PWD.Path),target=/workspace,readonly" --entrypoint sh personal-quant-lab-persistence_tests /workspace/test/offline/run.sh
```

This builds and runs all eight database-independent CTest suites, including the
fake-provider simulation. PostgreSQL does not need to run. Initial image/dependency
installation may need internet; subsequent execution uses the cached image.

For an already configured native build, `cmake --build build` then
`ctest --test-dir build -R FakeMarketDataTest --output-on-failure` runs the focused
suite. Native build dependencies must already be available for offline use.

Learning checkpoint: deterministic fixtures isolate financial mistakes from provider
outages. Own inputs, state exactly what each price means, and compare every financial
step to a numeric oracle. Passing fake-data tests does not prove real-data quality.
