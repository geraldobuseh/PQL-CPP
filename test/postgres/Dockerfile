FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ cmake ninja-build pkg-config libpqxx-dev libgtest-dev postgresql-client ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
ENTRYPOINT ["sh", "/workspace/test/postgres/run.sh"]
