# PQL-007: Quantitative correctness review

## Independent design decision

**QUANT REVIEW PASSED** for nonnegative-cash, long-only portfolio accounting with
atomic accepted-trade updates. This design decision precedes implementation validation.

## Invariants

Buy subtracts positive finite quantity-times-price only when sufficient cash exists.
Sell credits proceeds only when Position accepts its long-only reduction. A failed
cash or position operation changes neither state nor history. Keep flat positions
and their realized P&L. Starting cash plus accepted trades must reconstruct state.

Reject duplicate full-fill OrderIds across symbols and globally decreasing times;
same-time fills keep supplied order. Require every open-position mark and reject
ambiguous duplicates, with no price fallback. Aggregate in deterministic order.
Reject non-finite arithmetic, erased positive cash/value contributions and strict
overspends without epsilon forgiveness. Reuse Position's numeric invariants.

## Numerical oracle

Start $10,000; buy 10 SPY at $100 and 20 AAPL at $50: cash $8,000 and value $10,000
at fill marks. Sell 4 SPY at $120: cash $8,480. At marks SPY $120/AAPL $45, holdings
are $1,620 and total $10,100. Sell all AAPL at $45: cash $9,380, SPY value $720 and
unchanged total $10,100. Realized -$20 plus unrealized $120 equals $100 above starting
capital. These are synthetic accounting checks, not investment performance claims.

## Limits

Trade lacks portfolio attribution and costs. Trusted execution must route correctly;
fees/slippage, timestamped mark validation and durable history remain future work.
Absent costs do not imply execution is free. No strategy benchmark is applicable.

## Concrete implementation review

**QUANT REVIEW PASSED** after static inspection of source, tests and documentation.
No blocking financial findings. Cash, positions and accepted history commit together;
duplicates, invalid chronology, oversells, missing/duplicate marks and numerical
failures are rejected consistently. The numeric oracle reconciles correctly.

LOW coverage finding resolved: added a direct sale-credit regression for proceeds
swallowed by a large existing cash balance, and for old cash swallowed by large
proceeds. Root rebuilt and ran the regression and full CTest successfully. The Quant
Reviewer did not independently execute tests.
