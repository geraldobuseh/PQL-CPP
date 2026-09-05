# PQL-010 architecture review

Decision: ARCHITECTURE READY; final architecture and applicable quality review PASSED.

Bet: a small PostgreSQL schema can prevent malformed financial records while preserving deterministic ledger replay and existing C++ domain boundaries.

Root implemented; architect independently reviewed SQL and documentation. All 11 tables use explicit identities, constraints and relationships. Scoped order IDs and composite foreign keys enforce one full fill matching its request and one transaction matching its fill. Transactions/executions are append-only; starting state is protected; positions and snapshots remain projections. A per-portfolio sequence orders timestamp ties.

NUMERIC storage does not silently apply a fixed decimal scale. The current double-based C++ engine needs a deliberate future conversion contract. Aggregate correctness, writer serialization and role permissions remain outside this schema ticket and are documented in db/schema.md.

Resolved MEDIUM: repeat copying `db` could nest directories and execute stale files. Changed to `db/.`; root verified two copies into the existing destination. No unresolved material findings. Human merge decision remains separate.
