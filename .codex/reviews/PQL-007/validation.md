# PQL-007: Validation report

## Acceptance criteria

- PASS: Portfolio owns cash, positions and accepted Trade transaction history, plus
  identity and starting cash for reconstruction.
- PASS: applyTrade, marketValue, totalValue and cashBalance exist with the requested
  names and tested behavior.
- PASS: PortfolioSnapshot has no applyTrade or conversion to authority; container
  access is const. Compile-time tests enforce this boundary. A strategy stub only
  proposes an Order, and runtime tests prove the authoritative state is unchanged.

Actual strategy framework integration does not exist yet. Its required input is
Snapshot, never Portfolio or a mutation callback. This constraint remains binding.

## Executed commands

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
& build/test/pql_portfolio_tests.exe
& build/test/pql_portfolio_tests.exe --gtest_filter=PortfolioNumericTest.RejectsSaleCreditsThatLoseEitherCashContribution
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-format.exe' --dry-run --Werror src/domain/portfolio.hpp src/domain/portfolio.cpp test/test_portfolio.cpp
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

MSVC Debug build passed with warnings treated as errors. Initial portfolio execution
passed 21/21 tests. Independent Validator ran CTest (5/5 registrations passed) and
the portfolio executable (21/21 passed), checking the exact acceptance criteria.

Quant review identified a LOW test gap for swallowed sale credits. Root added one
test covering both small proceeds and small previous cash, rebuilt successfully and
ran the targeted regression (1/1 passed) and full CTest again (5/5 passed). The final
portfolio suite contains 22 cases. Formatting and tracked whitespace checks passed.

## Financial and failure evidence

Synthetic tests reconcile starting $10,000 with final cash $9,380 plus holdings $720
= total $10,100. Realized -$20 plus unrealized $120 equals the $100 increase.
Other cases verify exact-cash purchases, fractional trades, weighted costs, retained
flat positions, reopening and history field preservation.

Rejected insufficient cash, oversells, duplicates, chronology failures and arithmetic
failures leave complete snapshots unchanged. Replay reconstructs identical state
or rejects inconsistent history. Missing/duplicate marks reject full valuations;
quote order does not change stable position aggregation. Numerical tests cover
notional/cash/aggregate overflow, positive underflow and erased contributions.

Snapshot tests verify restricted API, independent lifetime, source coherence after
copy/move and that trusted application is separate from strategy order proposals.

## Findings, risk and debt

No unresolved blocking findings. LOW coverage gap resolved. Existing domain behavior
has low regression risk; prior PQL-004 through PQL-006 work was preserved.

Allocation failure safety was inspected structurally, not fault-injected. Sanitizers,
Release and other compilers/platforms remain **unverified**. Durable persistence,
costs, external cashflows, order attribution and market-mark freshness are outside
this ticket. Future strategy integration must preserve snapshot-only inputs.

## Decision

**VALIDATION PASSED**. Independent Validator recommendation: accept.
