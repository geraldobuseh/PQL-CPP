# PQL-014 architecture review

Decision: ARCHITECTURE READY; final architecture/C++ static review PASSED.

Architect independently reviewed HTTP/JSON isolation, ingestion CLI, repository
operation, migration and test wiring. The engine still consumes MarketDataProvider.
The optional Alpha Vantage target owns curl/JSON dependencies; the CLI completes
fetch/validation before opening PostgreSQL work. One symbol is the atomic unit.

storeDailyBars uses a row lock plus insert-or-compare on the full identity. Identical
reruns are unchanged; conflicting revisions reject without overwriting facts.
Session dates and canonical midnight keys are explicit and distinct from availability.
Success counts are logged after commit. No ORM, queue, scheduler or backtest added.

Coverage is deliberately bounded to the compact completed-day window. Current-day
rows are excluded after validation. Provider instances pace sequential queries by
2s; this is not an account-wide or distributed rate limiter. No material findings
remain. Runtime/live evidence is recorded separately in validation.md.
