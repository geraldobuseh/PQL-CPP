# PQL-022: Quantitative Correctness Review (Example Template)

**Ticket**: Example  
**Date**: [YYYY-MM-DD]  
**Reviewer**: Quant Agent  

## Financial Assumptions

[What are the financial model assumptions embedded in this implementation?]

- Portfolio NAV = cash balance + sum of (current price × quantity) for all positions
- Cost basis = sum of purchase prices × quantities for each position
- Realized P&L = proceeds from sold positions minus cost basis of sold shares
- Unrealized P&L = current market value minus cost basis of remaining shares
- Prices are point-in-time snapshots; no time interpolation between price updates
- Corporate actions (splits, dividends) are handled separately; not in NAV calculation
- Multi-currency portfolios: all prices assumed in base currency

## Invariants

[What financial constraints must hold at all times?]

- ✓ Cash flow conservation: sum of all transaction cash effects equals current cash balance
- ✓ Quantity conservation: sum of buy quantities equals sum of sell quantities plus current holdings
- ✓ Position cost basis never negative
- ✓ P&L cannot exceed available cash (no hidden leverage)
- ✓ NAV = cash + sum(price × quantity) for all positions

## Bias / Leakage Risks

[What could make this calculation financially incorrect?]

**Look-ahead bias**: 
- Risk: Using future prices to compute past valuations
- Mitigation: All price queries must be time-stamped; never use latest price for historical valuation

**Survivorship bias**: 
- Risk: Excluding delisted securities from portfolio value
- Mitigation: Include delisted positions at last known price; document assumption

**Data snooping**: 
- Risk: Optimizing portfolio valuation to match historical returns
- Mitigation: Valuation method is independent of strategy; no fitting

**Rounding errors**: 
- Risk: Decimal arithmetic accumulates errors in large portfolios
- Mitigation: Use fixed-precision arithmetic; never round intermediate values

**Missing prices**: 
- Risk: Silently using zero for securities without price data
- Mitigation: Fail explicitly; mark position as UNPRICED; do not include in NAV

## Required Edge Cases

[What scenarios must we test to ensure correctness?]

1. **Empty portfolio**: NAV = cash balance
2. **Zero shares**: Position should not appear in valuation
3. **Negative cash**: Portfolio still computes (margin scenario)
4. **Missing price data**: Position marked UNPRICED; raises error
5. **Same-day buys and sells**: Must correctly net quantities
6. **Very small prices**: No underflow; decimal precision preserved
7. **Corporate action (split)**: Quantity adjusted; cost basis per-share adjusted
8. **Delisted security**: Use last known price; document assumption

## Benchmark Requirements

[What comparison proves the calculation is correct?]

For v0.1:
- SPY benchmark (S&P 500 total return)
- Simple buy-and-hold strategy to verify NAV calculation against known returns
- Hand-calculated portfolio for small synthetic dataset

Never claim alpha without excess return versus appropriate benchmark.

## Findings

### CRITICAL

**None** — Implementation proposal is financially sound.

### HIGH

**None** — No material financial correctness issues identified.

### MEDIUM

None identified.

### LOW

**Assumption: Multi-currency handling deferred**
- Current implementation assumes single base currency
- Document clearly; raise ticket if multi-currency needed
- Severity: LOW (acceptable for v0.1 MVP)

## Decision

**✓ QUANT REVIEW PASSED**

The proposed valuation method is financially sound:
- Assumptions are explicit and defensible
- Invariants are mathematically clear
- Edge cases are well-understood
- Required tests are concrete
- Risks are documented

Proceed to implementation with confidence that financial correctness is not at risk.

---

*Save this template to `.codex/reviews/PQL-022/quant.md` when processing a financial ticket.*
