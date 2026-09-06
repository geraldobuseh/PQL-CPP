# PQL-011 validation

Decision: VALIDATION PASSED. Validator independently inspected coverage/code and
ran diff whitespace checks. Root supplied actual PostgreSQL runtime evidence.

Acceptance criteria:

- PASS: libpqxx 7.10.0 linked by optional pql_persistence target.
- PASS: SQL isolated from domain code behind three repository interfaces.
- PASS: real database integration proves inserts, reads across connections,
  legitimate portfolio-name update, destructor rollback, SQL-failure rollback,
  domain-failure rollback and mid-append rollback without orphan rows.

Executed on 2026-09-05:

```powershell
docker compose -f compose.yaml -f compose.test.yaml build persistence_tests
docker compose -f compose.yaml -f compose.test.yaml run --rm persistence_tests
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

Runner creates/migrates pql_integration_test, builds with GCC14.2/C++20 warnings as
errors and libpqxx7.10, and runs CTest against PostgreSQL17. Final result: 7/7 CTest
suites passed, including 13 integration cases. Test fixtures are confined to the
disposable database, which the runner drops on exit.

Additional passing cases: fee-aware reconstruction and equal-time event order;
pre-epoch millisecond quotes; missing lookups; competing purchases; duplicate and
mismatched requests; embedded NUL rejection; canonical numeric precision rejection;
minimum subnormal/maximum finite double round trips; imported overflow/underflow;
invalid OHLC and unsupported timestamps. Tests exercise interfaces and independently
query for orphan rows rather than only asserting that exceptions were thrown.

Failures resolved during validation:

- Debian libpqxx lacks CMake config metadata: added pkg-config fallback.
- Test compilation: honor nodiscard and put GoogleTest throw macros on separate lines.
- External-decimal fixture parameter inferred incompatible SQL types: explicit text casts.
- Review MEDIUM NUL truncation: input guards and rollback/alias regression.
- Review LOW indefinite wait: bounded readiness/CTest and database test timeouts.

Debt: native Windows adapter linking, sanitizers, interrupted commit/connection-loss
fault injection and a production permissions deployment remain unverified. No
automatic commit retry is offered. Market correction and projection writes are
out of scope. Root did not change domain implementation for this ticket.
