# PQL-022: C++20 Code Review (Example Template)

**Ticket**: Example  
**Date**: [YYYY-MM-DD]  
**Reviewer**: C++ Reviewer  
**Files reviewed**: `src/portfolio/valuation.hpp`, `src/portfolio/valuation.cpp`

## Ownership & Lifetime

**Finding: CLEAR ✓**

```cpp
double compute_nav(
    const std::vector<Transaction>& transactions,
    const PriceFeed& price_feed
) noexcept;
```

- Transactions passed by const reference: caller retains ownership
- PriceFeed passed by const reference: no ownership transfer
- Return value (double) is value semantics: clear ownership
- No dangling references or lifetime issues

RAII handling of temporary objects is explicit.

## Undefined Behavior

**Finding: NONE IDENTIFIED ✓**

Checked for:
- ✓ No dangling pointers (no pointers used at all)
- ✓ Iterator use is bounds-checked (standard library algorithms)
- ✓ All values initialized before use
- ✓ Integer overflow: quantities and prices use appropriate types (no signed/unsigned mixing)
- ✓ Safe casts: decimal prices cast with explicit `static_cast<>`
- ✓ No data races: single-threaded implementation; no mutable global state

## RAII & Exception Safety

**Finding: STRONG EXCEPTION GUARANTEE ✓**

```cpp
double compute_nav(
    const std::vector<Transaction>& transactions,
    const PriceFeed& price_feed
) noexcept;
```

- Function marked `noexcept`: throws no exceptions
- All operations are atomic or roll-forward only (no partial state)
- If price_feed throws, transaction history unchanged
- No temporary state that needs cleanup

**Recommendation**: Keep `noexcept` guarantee. If future refactors add complex logic, explicitly document what can throw.

## Const Correctness

**Finding: CORRECT ✓**

- Transactions vector: `const&` (read-only)
- PriceFeed: `const&` (read-only)
- Internal loop variables: const where appropriate
- Function result: value (not reference to internal state)

No unnecessary mutation. API is const-correct.

## Value / Move Semantics

**Finding: OPTIMAL ✓**

```cpp
double compute_nav(...)  // double = return value (cheap copy)
std::vector<Transaction>& transactions  // reference (no copy)
```

- Small return value (double): value semantics is optimal
- Large input (vector): passed by const reference (no copy)
- No unnecessary moves; implementation is straightforward
- Future optimization: if vector is very large, move semantics could be added if needed (not premature)

## APIs

**Finding: CLEAR ✓**

Strengths:
- Function name is explicit: `compute_nav` clearly indicates purpose
- Parameters are self-documenting: `transactions`, `price_feed`
- Deterministic: no hidden state, no time-dependent behavior
- No boolean flags ("anti-pattern" avoided)

Recommendation: Document in header whether `transactions` must be sorted by date. Add `require()`s if needed.

## Containers

**Finding: APPROPRIATE ✓**

- `std::vector<Transaction>`: correct choice for ordered sequence
- No performance issues identified for expected transaction counts
- Iterator stability: not relied upon

No container concerns.

## Performance

**Finding: NO ISSUES AT CURRENT SCALE**

Analysis:
- Time complexity: O(n) where n = number of transactions
- Space complexity: O(1) (no allocations beyond input)
- For current backtests (1-10K transactions): negligible cost
- **Unverified**: performance at 1M+ transactions

**Recommendation**: Add performance test for large portfolios. Current implementation adequate for MVP. If profiling shows NAV is bottleneck, add result caching before optimize computation.

## Summary

| Area | Status | Notes |
|------|--------|-------|
| Ownership | ✓ CLEAR | No ownership issues; RAII correct |
| Lifetime | ✓ SAFE | No dangling references; const refs preserve safety |
| Undefined behavior | ✓ NONE | Integer overflow checked; bounds verified |
| Exception safety | ✓ STRONG | `noexcept` function; atomic operations |
| Const correctness | ✓ CORRECT | All inputs const; no unnecessary mutation |
| Value semantics | ✓ OPTIMAL | Return small value; inputs passed by reference |
| API clarity | ✓ GOOD | Function name clear; parameters documented |
| Containers | ✓ APPROPRIATE | Vector is correct choice |
| Performance | ✓ ADEQUATE | O(n) acceptable; monitor at scale |

## Decision

**✓ C++ REVIEW PASSED**

Implementation demonstrates professional C++20 practices:
- Clear ownership and lifetime semantics
- Strong exception safety
- Correct const correctness
- No undefined behavior
- Optimal value/move semantics
- Performance adequate for MVP

Merge with confidence. Monitor performance if transaction volume grows beyond 1M.

---

*Save this template to `.codex/reviews/PQL-022/cpp-review.md` when completing C++ review.*
