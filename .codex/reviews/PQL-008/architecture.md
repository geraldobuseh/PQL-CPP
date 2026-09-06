# PQL-008: Architecture review

## Independent decision

**ARCHITECTURE READY** for immutable Transaction owning PortfolioId, Trade and
nonnegative fees, with typed read access and deleted assignment. Transaction history
replaces Trade history; there is one applyTransaction/replay path, not parallel sources.

## Required implementation

Validate portfolio attribution, chronology, duplicate full-fill OrderId, cash and
Position results before atomic commit. Existing applyTrade may remain a zero-fee
adapter constructing a Transaction for the receiving portfolio. Preserve detached
strategy snapshots. Position gains fee-aware symbol-level accounting without taking
on portfolio attribution or ledger ownership.

## Financial agreement and validation

Quant-approved policy capitalizes buy fees and deducts sell fees from proceeds and
realized P&L, including zero/negative proceeds when cash permits. Verify the zero-fee
oracle ($920 cash, one share, $20 realized, $1,040 total at $120 mark) and a fee-bearing
oracle, immutability, attribution rejection, atomic failures and replay equivalence.

No database, cashflow/correction framework or full event-sourcing infrastructure is
needed. The note must distinguish immutable facts, derived state and future durability.
