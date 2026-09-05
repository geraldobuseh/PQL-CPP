# PQL-008: Validation report

## Acceptance criteria

- PASS: Transaction owns portfolio attribution, symbol, side, quantity, price,
  explicit fees and timestamp, plus the originating OrderId.
- PASS: No public assignment or mutable field access; compile-time checks enforce
  immutability and read-only trade/symbol access. Copy/move construction preserves facts.
- PASS: Portfolio history contains Transaction records and reconstructs state through
  the same applyTransaction validation path used for accepted live updates.
- PASS: The $1,000 / BUY 2 AAPL at $100 / SELL 1 at $120 example and the requested
  learning checkpoint are implemented with correct numeric explanations.

## Commands actually executed

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
& build/test/pql_transaction_tests.exe
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-format.exe' --dry-run --Werror src/domain/transaction.hpp src/domain/transaction.cpp src/domain/position.hpp src/domain/position.cpp src/domain/portfolio.hpp src/domain/portfolio.cpp test/test_position.cpp test/test_portfolio.cpp test/test_transaction.cpp
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

MSVC Debug compilation passed with warnings treated as errors. All 6 CTest registrations
passed, including prior primitive, order/trade, position and portfolio regressions.
The new transaction executable passed 13/13 tests. Changed C++ format and tracked
whitespace checks passed.

Independent Validator also executed CTest (6/6 passed) and the transaction executable
(13/13 passed), inspecting source, tests, documentation and the exact ticket criteria.

## Numerical and boundary evidence

Zero fees at supplied $120 mark: cash $920, one AAPL share, $100 basis, realized $20,
market value $120, unrealized $20 and total value $1,040. Replaying again returns the
same complete snapshot. Missing marks reject valuation.

Buy fee $2/sell fee $1: cash $917, one share at $101 average, realized $18, unrealized
$19 and total value $1,037. Live application and replay agree exactly, including fees
and all stored transaction fields.

Other tests cover negative fees, weighted fee-bearing purchases, partial/full sales,
wrong portfolio, duplicate identities with changed fees, insufficient cash caused by
fees, zero/negative sale proceeds, exact realized-profit cancellation, overflow and
lost fee contributions. Rejected updates preserve complete snapshots. Snapshot has
no applyTransaction API; existing strategy-boundary regressions remain green.

## Test migration and regression risk

Earlier portfolio tests now expect attributed zero-fee Transaction history rather
than Trade history, retaining all original accounting expectations. The Position
compile-time restriction uses a requires check because with_trade is now overloaded.
These are required interface migrations, not weakened financial expectations.

History representation and fee calculations changed, so these are material domain
boundaries. No blocking findings remain after independent reviews and regression tests.

## Remaining validation debt

Allocation failure safety was inspected, not fault-injected. Sanitizers, Release and
other compiler/platform configurations remain **unverified**. Durable persistence,
correction/cashflow events, partial fills, accounting versioning and market-mark
freshness are explicitly outside the implemented scope.

## Decision

**VALIDATION PASSED**. Independent Validator recommendation: accept.
