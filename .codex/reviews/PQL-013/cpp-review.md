# PQL-013 C++20 review

Decision: C++ REVIEW PASSED after explicit ownership policy.

Architect reviewed the fixture map and query paths. Failed construction destroys
partial state. No borrowed input/returned data or raw owning pointers persist.
Validated primitives protect OHLCV values; per-symbol date checks protect sequences.

Generated assignment was removed because it has no class-level strong exception
guarantee for cross-element invariants under allocation failure. No allocator fault
was reproduced; this is a preventive API restriction. Default copy construction
also copies from rvalues and preserves the source, following existing domain policy.
Traits and runtime ownership tests cover that policy. No remaining material findings.
