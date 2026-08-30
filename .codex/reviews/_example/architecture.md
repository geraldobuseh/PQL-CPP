# PQL-022: Architecture Review (Example Template)

**Ticket**: Example  
**Date**: [YYYY-MM-DD]  
**Reviewer**: Architecture Agent  

## Objective

[What capability is actually required? Be specific about scope.]

Example: "Implement a deterministic portfolio valuation method that computes NAV given a transaction history and current market prices."

## Current Architecture

[What components already exist? What do they do? What are their boundaries?]

Example:
- `Portfolio` class holds positions and cash balance
- `Transaction` class records executed trades (immutable)
- `PriceFeed` provides market data (synchronized snapshots)
- Portfolio state currently computed from position cache (not from transactions)

## Proposed Design

[What is the smallest coherent change that solves the problem?]

Example:
- Add `compute_nav(transactions, prices)` pure function
- Replay transactions chronologically to rebuild position state
- Apply market prices for unrealized P&L
- Preserve existing Portfolio cache for performance
- New function does not mutate portfolio

## Contracts

[What interfaces or domain contracts are affected?]

Example:
- `Portfolio` read-only for valuation
- `Transaction` must be append-only and immutable
- `PriceFeed::get_price(symbol, date)` must return consistent results for same query
- New valuation API must be deterministic: same inputs → same output

## Invariants

[What must remain true?]

Example:
- Cash flow is conserved: sum of all transactions' cash effects must equal portfolio cash
- Cost basis is never negative
- Unrealized P&L = (current price - average cost) × quantity
- Portfolio valuation = cash + sum of position valuations

## Required Work

[What is necessary for this ticket?]

1. Implement `compute_nav()` function
2. Add tests for cash conservation
3. Add edge case tests (zero shares, missing prices, negative cash)
4. Update documentation

## Adjacent Work

[What is useful but not required?]

- Optimize position reconstruction (currently O(n) per query)
- Add caching layer for common price queries
- Refactor Portfolio to use computed valuation

## New Bets

[What discoveries belong outside this ticket?]

- Handling corporate actions (splits, dividends)
- Multi-currency portfolios
- Intraday transaction batching

## Risks

[What could materially go wrong?]

- **Financial**: Rounding errors in decimal arithmetic accumulate over time
- **Correctness**: Missing edge case in transaction ordering (e.g., same-day transactions)
- **Performance**: Replaying all transactions for every valuation query becomes expensive at scale
- **Invariant violation**: If transactions are modified after creation, valuation becomes incorrect

## Validation Requirements

[What evidence would prove the implementation is correct?]

- [ ] Synthetic portfolio with known cash flows: valuation matches hand-calculated expectation
- [ ] Edge cases: zero positions, negative cash, missing prices, duplicate transactions
- [ ] Cash conservation test: verify sum of transaction effects equals cash balance
- [ ] Determinism test: multiple calls with same inputs produce identical results
- [ ] Regression: existing portfolio tests still pass

## Decision

**✓ ARCHITECTURE READY**

The proposed design is minimal, preserves existing invariants, and clearly separates concerns (computation vs. storage). Risks are well-understood and validation requirements are concrete.

---

*Save this template to `.codex/reviews/PQL-022/architecture.md` when processing a substantial ticket.*
