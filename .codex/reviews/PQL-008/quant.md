# PQL-008: Quantitative correctness review

## Independent design decision

**QUANT REVIEW PASSED** for the proposed fee-aware, attributed transaction contract.

## Invariants and oracle

Buy cash debit: quantity times price plus fees. Capitalize fees into basis.
Sell cash change: quantity times price minus fees. Realized increment: quantity
times price-minus-average minus fees. Partial sales preserve average. Negative fees
and wrong portfolio attribution reject. Zero net proceeds are valid; negative net
proceeds require existing cash. Replay keeps atomicity, chronology and deduplication.

Starting $1,000, buy 2 AAPL at $100 with fee $2: cash $798, average $101 and basis
$202. Sell 1 at $120 with fee $1: cash $917, one share, realized $18. At an explicit
$120 mark, unrealized $19 and total value $1,037. Gain $37 equals realized plus
unrealized, and also $40 gross gain minus $3 total fees.

## Required edge cases

Zero/negative fees, mixed-fee weighted buys, partial/full sales, fee-driven insufficient
cash, zero/negative net proceeds, exact cancellation of gross profit, wrong portfolio,
replay equivalence, overflow and wholly swallowed fee contributions. Past execution
prices do not establish a current market mark. No strategy benchmark is applicable.

## Concrete implementation review

**QUANT REVIEW PASSED**, with no blocking financial findings after inspecting source,
13 new test definitions and the learning note. Buy fee capitalization, sell fee
subtraction, valid cancellation, fee-driven net debits with cash coverage, attribution
and atomic replay match the contract. Fee-free total $1,040 and fee-adjusted total
$1,037 reconcile with their realized/unrealized amounts. No further required edge
cases were identified. This review was static; executed results are in validation.md.
