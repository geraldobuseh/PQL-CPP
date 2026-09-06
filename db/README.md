# v0.1 database migrations

Start the PostgreSQL service using the root README instructions, then run from the repository root:

```powershell
docker compose cp db/. postgres:/tmp/pql-db
docker compose exec -T postgres psql -X -U pql -d personal_quant_lab -f /tmp/pql-db/migrate.sql
```

The psql driver applies ordered migrations and records their versions in the same transaction. An advisory transaction lock serializes concurrent migration attempts. Errors abort the migration; rerunning skips recorded versions. Never edit an applied migration: add a numbered migration and its driver entry instead. There is no automatic destructive rollback.

Run constraint tests against a disposable migrated database:

```powershell
docker compose exec -T postgres createdb -U pql pql_schema_test
docker compose exec -T postgres psql -X -U pql -d pql_schema_test -f /tmp/pql-db/migrate.sql
docker compose exec -T postgres psql -X -U pql -d pql_schema_test -f /tmp/pql-db/tests/schema.sql
docker compose exec -T postgres dropdb -U pql pql_schema_test
```

Check each command's exit code before continuing. Tests roll back their fixtures. Only drop the disposable database you created for testing.

See [column constraints](schema.md) for assumptions, invalid states, and rules that require the future persistence writer. This ticket creates schema, not a C++ database adapter or role/permission deployment.
