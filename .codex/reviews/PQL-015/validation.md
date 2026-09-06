# PQL-015: Validation Report

VALIDATION PASSED. Validator static review found no blockers; runtime evidence below
was obtained by the root implementer.

All acceptance criteria pass: fixtures reject missing/null/blank and negative OHLC
prices, duplicate dates, future session/metadata dates and impossible OHLC bounds.
Additional cases cover zero/nonfinite/unrepresentable prices, invalid calendar dates
and volumes. Invalid current-day/out-of-range bars still reject without retry.
A PostgreSQL integration test confirms an invalid response persists no good prefix.
The required notes/005-data-quality.md includes numeric examples and validation limits.

Executed:

```powershell
docker compose -f compose.yaml -f compose.test.yaml run --rm persistence_tests
```

Result: clean C++ build, both schema constraint scripts passed, all11 CTest suites
passed (including PostgreSQL and daily ingestion integration suites). Alpha Vantage
has13 fixture tests; daily ingestion has5 integration tests. No live API calls.

Remaining validation debt: CLI stdout JSON rendering is not directly asserted by
an automated test; underlying typed diagnostic context is tested. Live API behavior
was not reverified for this ticket. Exchange-calendar holes, freshness, corporate
actions and point-in-time availability remain outside this ticket's scope.
