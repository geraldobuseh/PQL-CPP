# PQL-005: Quantitative correctness review

## Final independent review

**QUANT REVIEW PASSED** — no blocking objection to the bounded request/record
contract. This approval covers domain models, not executable trading.

## Invariants and assumptions

- Positive finite order quantities, including fractions, for both Buy and Sell.
- Primitive Quantity rejects negatives and non-finite values; Order rejects zero.
- Validated positive Price and execution time at or after submission.
- Trade derives originating identity, symbol, side and full-fill quantity from Order.
- No price selection, wall-clock access, balance mutation or return calculation.

Pending-only status and omitted portfolio/strategy attribution and fees are accepted
for this ticket. Costs are not assumed to be zero. Exact notional and fee/slippage
accounting remain future execution/ledger work.

## Findings and required boundaries

MEDIUM boundary: a positive sell request does not prevent a short sale. Execution
must check available holdings before filling. Trade construction does not authorize
execution, check holdings/cash, prevent duplicate fills or transition Order status.
This is documented in code and the ticket contract; no short-selling mechanism is
implemented here.

LOW boundary: absent fee fields must not be interpreted as free execution.

## Required evidence

Buy/sell creation, zero (including negative zero), invalid numeric/enum inputs,
fractional quantities, field consistency, equal/later execution time acceptance and
earlier execution time rejection. No benchmark comparison applies to value models.
