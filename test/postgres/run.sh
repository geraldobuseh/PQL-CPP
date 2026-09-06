#!/bin/sh
set -eu
# Never reuse or drop an existing database. createdb must succeed before cleanup
# is armed. Concurrent test runs fail explicitly instead of sharing fixtures.
test "$PGDATABASE" = pql_integration_test
createdb "$PGDATABASE"
trap 'dropdb --if-exists "$PGDATABASE"' EXIT
psql -X -f /workspace/db/migrate.sql
psql -X -q -f /workspace/db/tests/schema.sql
psql -X -q -f /workspace/db/tests/daily_market_prices.sql
cmake -S /workspace -B /tmp/pql-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DPQL_ENABLE_SANITIZERS=OFF \
    -DPQL_BUILD_PERSISTENCE=ON -DPQL_BUILD_POSTGRES_TESTS=ON -DPQL_BUILD_REAL_MARKET_DATA=ON
cmake --build /tmp/pql-build -j 2
ctest --test-dir /tmp/pql-build --output-on-failure
