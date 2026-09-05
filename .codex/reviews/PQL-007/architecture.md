# PQL-007: Architecture review

## Objective and current surface

Add Portfolio to `pql_domain`, composing existing Position/Trade primitives. The
repository has no strategy, broker, ledger schema or database integration to change.
Required operations retain the ticket names applyTrade, marketValue, totalValue and
cashBalance. The strategy mutation boundary is non-negotiable.

## Independent design decision

**ARCHITECTURE READY** for an owning Portfolio and detached PortfolioSnapshot.
Snapshot is the required future strategy input, with no trade application or reference
back to authority. Trusted execution owns Portfolio. No speculative engine/key/token
framework is required for this ticket.

Keep starting cash, current cash, positions including flats, and accepted Trade history.
Use vectors for deterministic small-scope storage without introducing symbol hashing
or ordering. Typed MarketPrice inputs reject duplicate and missing open-position marks.
Replay uses the same acceptance path as normal trade application.

## Required invariants

Validate no margin/shorting, unique full-fill OrderId, global chronology and numeric
representability. Update position, cash and history atomically via candidate-copy
and no-throw commit. Source state survives any rejection/allocation failure.

## Evidence required and limits

Compile-time tests prove Snapshot cannot apply trades or expose writable containers;
runtime tests prove detached lifetime and that order proposals do not change authority.
Test accounting, accepted history, unchanged rejection, replay, missing/duplicate
marks and numerical failures. Future strategy wiring must use Snapshot exclusively.
Persistence, external cashflows, fees, live brokerage and strategy execution stay out.
