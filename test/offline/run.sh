#!/bin/sh
set -eu
# Uses already installed compiler and GoogleTest dependencies. No DB or network.
cmake -S /workspace -B /tmp/pql-offline-build -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug -DPQL_ENABLE_SANITIZERS=OFF \
    -DPQL_BUILD_PERSISTENCE=OFF -DPQL_BUILD_POSTGRES_TESTS=OFF
cmake --build /tmp/pql-offline-build -j 2
ctest --test-dir /tmp/pql-offline-build --output-on-failure
