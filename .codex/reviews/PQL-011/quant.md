# PQL-011 quantitative correctness review

Decision: QUANT REVIEW PASSED for implemented paths. Quant agent independently
reviewed SQL, domain interactions and test definitions; root ran database tests.

Portfolio row locking serializes cooperating writers. Replay and applyTransaction
reuse fee, cash, ownership, chronology and duplicate guards. Original request time
and fill time are distinct persisted facts. Failed multi-row writes roll back.

Shortest round-trip decimal serialization plus numeric equality on read supports
ordinary engine values while rejecting imported decimals whose precision would be
lost. Integer clock conversion preserves milliseconds, including pre-epoch values.
Prices never replace ledger execution prices or authoritative starting cash.

Root's passing evidence includes $1,000 seed, buy 2 at $100 with $2 fee, sell 1 at
$120 with $1 fee: $917 cash, 1 share, $18 realized profit, $1,037 value at $120 mark.
Competing $80 purchases against $100 cash permit only one committed purchase.

Earlier design discussion mentioned market corrections; final scope uses metadata
rename for legitimate update and documents market data as insert-only. No missing
acceptance criterion results. Strategy attribution, projection writes and revision
provenance remain separate work. No unresolved HIGH/CRITICAL financial findings.
