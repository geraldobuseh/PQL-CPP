# PQL-014: Alpha Vantage daily ingestion

The optional `pql_alpha_vantage` adapter implements MarketDataProvider for exactly
SPY, QQQ, AAPL, MSFT, NVDA, AMZN, GOOGL and META. It requests Alpha Vantage's
`TIME_SERIES_DAILY` compact JSON endpoint, which supplies raw daily OHLCV and the
latest 100 observations. Older/full history requires a different entitlement and
is not silently substituted. See [provider documentation](https://www.alphavantage.co/documentation/#daily)
and [quota information](https://www.alphavantage.co/support/).

## Run locally

Set `ALPHAVANTAGE_API_KEY` in the ignored `.env` alongside your existing PostgreSQL
password. Do not commit credentials or place keys in command arguments.

Apply migrations once (safe to rerun):

```powershell
docker compose up -d --wait postgres
docker compose cp db/. postgres:/tmp/pql-db
docker compose exec -T postgres psql -X -U pql -d personal_quant_lab -f /tmp/pql-db/migrate.sql
```

Ingest a recent range of completed sessions, replacing these dates as needed:

```powershell
docker compose -f compose.yaml -f compose.ingest.yaml run --build --rm ingest 2026-09-03 2026-09-04 all
```

Use `AAPL` instead of `all` to ingest one symbol. Append `--fetch-only` to validate
the real API without storing rows. Each symbol costs at least one API request;
retries and repeated runs also consume quota. Existing bars are not a cache for
provider requests. The wrapper builds the CLI in an ephemeral compiler container.

Native builds enable `PQL_BUILD_REAL_MARKET_DATA=ON` and `PQL_BUILD_PERSISTENCE=ON`.
Dependencies: libcurl>=7.85, nlohmann_json>=3.11 and the existing libpqxx dependency.
Set standard PG* variables and ALPHAVANTAGE_API_KEY; run
`pql_ingest_daily FROM TO [SYMBOL|all] [--fetch-only]`.

## Validation and date policy

The entire response is checked before filtering or opening a database transaction.
Duplicate JSON keys, wrong symbol/time zone, inconsistent last-refreshed metadata,
invalid dates, malformed/missing fields, nonpositive/nonfinite prices, bad OHLC bounds
and negative/fractional/unrepresentable volume reject. Provider error/quota envelopes
are errors even with HTTP200. Unordered JSON date members are emitted chronologically.

One UTC date is captured per run. Future rows reject; today's row is validated but
excluded. Latest means the newest accepted completed close, not a realtime quote.
History endpoints are inclusive and must both be within the returned completed-bar
window. Requests ending after the newest row (including weekends/holidays) reject
conservatively: choose actual available session endpoints. The adapter does not
independently detect missing interior exchange sessions, guarantee freshness, or
establish historical information availability. Raw prices are not total returns.

Financial text is validated against the exact same general shortest-round-trip
decimal representation used by persistence. Excess precision that would change
the recorded decimal rejects rather than being silently rounded. This does not
make the engine's double arithmetic exact decimal accounting.

## Durable identity and retries

Migration002 adds `market_prices.session_date`. Daily rows use the session label
and canonical UTC midnight in observed_at. Midnight is a database key, never an
exchange-close or data-availability timestamp. Source is `alphavantage.daily`,
adjustment is `raw`, currency is USD. OHLCV and date are persisted explicitly.

`MarketPriceRepository::storeDailyBars` registers missing assets, locks the asset
row and inserts the batch inside its owning unit of work. An existing identical
full-key row increments unchanged; a conflicting OHLCV value rejects the whole
symbol batch. There is no silent update, revision repair or partial symbol commit.
Database uniqueness also protects against competing writers. A failure after some
symbols committed is recoverable by rerunning the command: those symbols become
unchanged. Financial transactions/portfolio balances are not modified by ingestion.

Transport timeout/connect/send/receive failures and HTTP408/429/5xx retry at most
three total attempts, with 1s then2s backoff. Separate queries on one provider
instance are paced by2s to avoid bursts.
Provider instances are for sequential use; this is not an account-wide limiter.
Numeric Retry-After can extend a wait
up to30s. Larger or date-form delays stop the run for that symbol so it can be
retried later, rather than retrying too early. HTTP authentication/rejection,
malformed data, coverage errors, quota/entitlement JSON and conflicting revisions
are not automatically retried. Database failures roll back and can be rerun;
uncertain commit is explicitly reported without an automatic retry.

libcurl verifies TLS, does not follow redirects, restricts the endpoint to HTTPS,
uses 10s connect/30s total timeouts, and limits bodies to2MiB. HTTP never runs while
a database transaction holds locks. Diagnostic messages exclude URLs, keys and raw
response bodies.

## Observe and test

The executable emits JSON lines for ingestion_start, provider_request,
provider_retry, symbol_committed, symbol_failed, symbol_commit_uncertain and
ingestion_finished. Committed records include symbol, fetched/inserted/unchanged
counts and elapsed milliseconds; the run start includes the requested date range.
Exit0 means all requested symbols succeeded; nonzero means inspect the failure
events. Build output from the wrapper precedes these JSON events.

Validation failures emit `symbol_failed` with `quality_code`, `field` and
`session_date` (null when unavailable). Codes include `missing_price`,
`invalid_price`, `duplicate_date`, `future_timestamp`, `invalid_ohlc`,
`invalid_volume`, `invalid_date` and `malformed_response`. Missing, null and blank
OHLC fields are missing prices; zero, negative, nonfinite and unrepresentable
prices are invalid prices. Context contains only enum names and validated dates,
never raw provider values. The first detected issue rejects the entire response,
including bad rows outside the requested range; validation failures do not retry.

For example, a bar with high90 and low110 reports `invalid_ohlc` with field `high`.
Duplicate dates inside one response reject; an identical previously stored bar
on a later ingestion remains an idempotent no-op. See
[the data quality learning note](../notes/005-data-quality.md) for examples and limits.

Inspect stored daily rows:

```sql
SELECT a.symbol, p.session_date, p.open, p.high, p.low, p.close, p.volume
FROM market_prices p JOIN assets a USING (asset_id)
WHERE p.source = 'alphavantage.daily'
ORDER BY a.symbol, p.session_date;
```

Run automated tests (they use fixtures, not your API key):

```powershell
docker compose -f compose.yaml -f compose.test.yaml build persistence_tests
docker compose -f compose.yaml -f compose.test.yaml run --rm persistence_tests
```

This runs both schema constraint scripts, all unit suites and PostgreSQL integration
tests. Coverage includes all eight symbols, retries, quota/errors, malformed JSON,
date/precision boundaries, repeat imports, concurrent imports and conflict rollback.
Live API verification is separate evidence and consumes provider quota.
