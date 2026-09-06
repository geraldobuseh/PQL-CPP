#!/bin/sh
set -eu
cmake -S /workspace -B /tmp/pql-ingest-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DPQL_ENABLE_SANITIZERS=OFF \
    -DPQL_BUILD_PERSISTENCE=ON -DPQL_BUILD_REAL_MARKET_DATA=ON
cmake --build /tmp/pql-ingest-build --target pql_ingest_daily -j 2
exec /tmp/pql-ingest-build/src/pql_ingest_daily "$@"
