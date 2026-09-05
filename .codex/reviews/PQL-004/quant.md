# PQL-004: Quantitative correctness review

## Financial assumptions and invariants

Independent Quant Reviewer recommended finite signed Money, nonnegative Quantity
magnitudes (including fractional units and zero), and strictly positive equity
Price. The implementation follows these scoped contracts. All reject NaN and
infinities. Money/Price use approximate doubles in one externally agreed currency.

Timestamps use explicit system-clock instants at millisecond precision and accept
epoch, pre-epoch and future values. Symbols preserve accepted input exactly; IDs
are distinct positive signed 64-bit values. No silent normalization occurs.

## Bias, benchmark and edge cases

No strategy, performance calculation or benchmark is introduced; SPY comparison
does not apply. Timestamp creation never reads now, preserving determinism. Data
availability/future-data checks belong to later ingestion and simulation boundaries.

Required cases: NaN, infinities, zero, negatives, fractional quantities, malformed
symbols, invalid IDs, pre-epoch timestamps and compile-time domain separation.

## Findings and decision

**QUANT REVIEW PASSED** for these contracts. Exact accounting, currency conversion,
signed position quantities and zero-valued holdings need future domain decisions.
LOW adjacent finding: the existing README ticket roadmap is stale.
