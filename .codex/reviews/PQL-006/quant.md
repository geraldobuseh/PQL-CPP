# PQL-006: Quantitative correctness review

## Independent design decision

**QUANT REVIEW PASSED** for weighted-average, long-only research accounting.

Buys update average using total acquisition cost divided by total shares. Partial
sells preserve average. Exact liquidation clears basis and reopening starts fresh.
Market value is quantity times mark; unrealized is quantity times mark-minus-average.
Sales realize quantity-sold times fill-minus-average. Empty value/unrealized are zero.

## Numerical oracle

Buy 10 at $100 then 10 at $120: 20 shares, $110 average and $2,200 basis.
At mark $125: market value $2,500 and unrealized $300.
Sell 5 at $130: 15 shares, unchanged $110 average, $1,650 basis, realized +$100.
At mark $125: value $1,875 and unrealized +$225.
Sell 15 at $90: flat, no average, realized -$300 on that sale and -$200 cumulatively.

## Required protections and limitations

Reject mismatched symbols, oversells, non-finite calculations and share adjustments
lost to finite precision. Use exact represented comparisons without epsilon-based
oversell forgiveness or lot snapping. Positive value underflow must not silently
erase basis. Failed updates preserve source state. Approximate arithmetic may leave
fractional residuals; liquidation uses the reported owned quantity.

Costs, corporate actions, tax-lot accounting, multi-currency behavior and replay
deduplication are outside the current model. Fee-less Trade does not imply free
execution. No strategy or benchmark comparison applies to this accounting ticket.

## Required tests

Four critical lifecycle cases plus fractional quantities, empty valuation, unequal
buys, reopening, loss cases, overflow, swallowed quantity changes, strict oversell
rejection and immutable failure. Notes must explain the numeric example and limits.

## Concrete implementation review

The Quant Reviewer subsequently inspected the implementation and learning note:
**QUANT REVIEW PASSED**, with no blocking financial defects. Weighted buys, partial
sales, reset/reopen, realized accumulation, chronology and strict oversell checks
match the contract. The $2,200 purchase cost versus $2,000 proceeds reconciles with
a $200 cumulative realized loss.

One LOW comment clarification was resolved: a flat position has zero unrealized
P&L but can retain nonzero realized history. This was a static review; executed
numeric edge-test results are recorded in `validation.md`.
