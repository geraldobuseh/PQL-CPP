# PQL-010 validation

Decision: VALIDATION PASSED. Validator independently inspected acceptance coverage and SQL; root executed database checks on local PostgreSQL 17.

Acceptance evidence:

- PASS: all 11 requested tables exist, plus schema_migrations metadata.
- PASS: PK, FK, NOT NULL, UNIQUE and CHECK constraints protect appropriate columns.
- PASS: db/schema.md names every column and explains invalid states and nullable exceptions.

Executed by root on 2026-09-05:

- `docker compose cp db/. postgres:/tmp/pql-db` twice: top-level files refreshed correctly.
- Created disposable pql_schema_validation_010; ran `psql -X -U pql -d pql_schema_validation_010 -f /tmp/pql-db/migrate.sql`: success.
- Ran `/tmp/pql-db/tests/schema.sql` in that database: success; fixtures rolled back. Includes dynamic NOT NULL tests for all required non-generated domain columns, numeric special values, timestamp bounds, mismatched FKs, uniqueness, immutable UPDATE/DELETE/TRUNCATE, generated totals and the $1,000 ledger example.
- Reran migration: version count 1, public table count 12, assets count 0.
- Created disposable pql_schema_atomicity_010 with preexisting executions sentinel 42. Migration failed at the expected name collision. Only the original sentinel table remained; no partial schema or version metadata committed.
- Applied migration to personal_quant_lab: version 1 recorded, all 11 domain tables verified, transaction row count 0.
- Dropped both disposable databases successfully.

Resolved MEDIUM: stale files on repeat directory copy; content-copy command verified. Validator ran whitespace diff check successfully. No C++ source changed for this ticket; C++ tests were not rerun.

Validation debt: concurrent migration attempts were inspected (advisory transaction lock), not stress-tested. C++ persistence conversion, cross-row writer rules and production permissions are unimplemented and unverified. Tests validate stored constraints, not a PostgreSQL replay adapter or complete financial formula engine.
