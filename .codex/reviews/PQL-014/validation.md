# PQL-014 validation

Decision: VALIDATION PASSED. Validator independently inspected source/tests and
ran diff whitespace checks. Root executed database, compiler and live checks.

Acceptance evidence:

- PASS: all8 symbols supported by the adapter and independently verified in live
  PostgreSQL storage: SPY,QQQ,AAPL,MSFT,NVDA,AMZN,GOOGL,META.
- PASS: daily open/high/low/close/volume/session_date stored with provenance and
  daily consistency/uniqueness constraints.
- PASS: identical repeated/concurrent imports are unchanged, conflicting revisions
  roll back, retries are bounded/observable and CLI commits log counts.

Executed with the Docker compiler/PostgreSQL17 environment:

```powershell
docker compose -f compose.yaml -f compose.test.yaml build persistence_tests
docker compose -f compose.yaml -f compose.test.yaml run --rm persistence_tests
docker compose cp db/. postgres:/tmp/pql-db
docker compose exec -T -e 'PGOPTIONS=-c search_path=pg_catalog' postgres psql -X -U pql -d personal_quant_lab -f /tmp/pql-db/migrate.sql
docker compose -f compose.yaml -f compose.ingest.yaml run --build --rm ingest 2026-09-03 2026-09-04 all
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

Final tests: both SQL constraint scripts and11/11 CTest suites passed, including
7 Alpha Vantage tests and4 daily-ingestion tests. Coverage includes exact symbols,
schema/date identities, duplicate JSON, invalid fields/dates/precision, retries,
request pacing, errors without body leakage, all-symbol inserts/reruns, concurrency
and revision rollback. Tests use fixture transport, never the user's API quota.

Existing personal_quant_lab upgraded from version1 to2 with search_path=pg_catalog
at connection entry. Driver now pins public,pg_catalog itself. Rerun skipped both
applied migrations. This resolves the MEDIUM upgrade-path finding.

Live evidence (UTC2026-09-06): Release CLI with real curl/TLS and user-configured
local API key first committed2 SPY rows; the next7 requests returned quota/entitlement
responses. Sequential queries were then paced by2s. The subsequent live run returned
exit0 with all8 symbols successful: SPY unchanged2, other symbols inserted14 total.
Independent SQL confirmed each of8 symbols has exactly2 rows dated2026-09-03 and
2026-09-04, with schema versions1 and2. No additional live calls were made after
success. The observed recovery supports burst throttling as the first run's cause;
raw provider responses and credentials were not logged.

Other resolved findings: asset INSERT parameter type ambiguity; fixed/general
decimal mismatch reproduced and corrected with regression. No open blockers.

Remaining debt: independent exchange-calendar gap validation, full/premium history,
native Windows adapter build, sanitizers, real HTTP fault injection and commit-loss
fault injection. Fixture retries and real happy-path curl/CLI execution are verified.
