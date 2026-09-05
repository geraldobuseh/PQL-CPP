# PQL-006: Architecture review

## Objective and surface

Add a Position value to the existing domain library, using Order/Trade/primitives
from PQL-004/PQL-005. No portfolio, broker or ledger exists to migrate. The learning
checkpoint is `notes/003-cost-basis.md` with actual numeric examples.

## Independent design review

**ARCHITECTURE READY** for an immutable, replay-derived Position with empty(Symbol),
with_trade(Trade) returning an optional new Position, typed accessors and supplied
market Price valuation. Store quantity, optional average cost, cumulative realized
Money and optional last fill timestamp, with owned Symbol storage.

Weighted-average buys update cost; partial sells preserve average exactly. Exact
liquidation clears average and quantity while preserving realized history. Reopening
uses the new fill price. Derived cost/value calculations must be checked.

## Required invariants

Reject symbol mismatch, oversell, decreasing replay time, non-finite results,
underflow that erases positive value and quantity changes lost to rounding. Equal
timestamps keep caller order. Rejected updates leave the input Position unchanged.

No financial history vector, deduplication index, mutable portfolio balances, broker,
price fetcher, fee model or tax-lot machinery is introduced. Unique ordered fills
belong to the eventual source-of-truth ledger; marks need caller-controlled freshness.

## Validation requirements

Hand-calculated first/additional buys, unequal-size weighting, partial/full sells,
reopening, fractional shares, gains/losses, deterministic replay, immutable failure,
chronology and numeric boundaries. The learning note must distinguish remaining
basis from realized and unrealized P&L and explain the research-accounting scope.
