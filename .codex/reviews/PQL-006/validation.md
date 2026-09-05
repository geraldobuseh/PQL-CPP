# PQL-006: Validation report

## Acceptance criteria

- PASS: Position exposes owned Quantity and optional weighted-average Price.
- PASS: Supplied market Price produces checked market value and unrealized Money.
- PASS: Explicit tests cover first purchase, additional purchase, partial sale and
  complete liquidation.
- PASS: `notes/003-cost-basis.md` explains cost basis, realized and unrealized P&L
  with worked numeric examples and all six learning-checkpoint questions.

## Actual commands and results

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
& build/test/pql_position_tests.exe
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-format.exe' --dry-run --Werror src/domain/position.hpp src/domain/position.cpp test/test_position.cpp
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

MSVC Debug build passed with warnings treated as errors. CTest passed all 4
registrations: smoke, financial primitives, order/trade and position. The position
executable passed all 20 tests. Changed C++ formatting and tracked whitespace
checks passed.

Independent Validator reran CTest (4/4 passed) and the position executable (20/20
passed), then mapped the exact user acceptance criteria to source, tests and note.

## Financial evidence and failure paths

Independent arithmetic confirms the primary scenario: buys 10 at $100 and 10 at
$120 produce $110 average. Selling 5 at $130 leaves $1,650 basis and realizes $100;
the remaining shares have $225 unrealized at mark $125. Selling the rest at $90
leaves cumulative realized -$200 and zero remaining basis/value/unrealized.

Additional evidence covers unequal buy weighting, fractional holdings, losses,
mark changes, reopening, symbol mismatch, strict oversells, decreasing times,
equal-time replay, deterministic results and unchanged source state on rejection.
Numerical tests cover overflowing trade values, quantity and basis additions,
valuation/sell notional, realized accumulation, swallowed contributions, positive
value/profit underflow and exact fractional liquidation using reported holdings.

## Findings and regression risk

No blocking defects found. One LOW comment ambiguity about flat-position P&L was
corrected and independently verified: realized history remains after liquidation.
The model is additive and performs no cash or persistence mutation. Earlier
PQL-004/PQL-005 workspace changes were preserved.

## Remaining validation debt

Release, sanitizers and other compiler/platform configurations remain **unverified**.
Fees, cash accounting, durable replay/deduplication, corporate actions and mark
freshness are outside this ticket and not claimed as validated.

## Decision

**VALIDATION PASSED**. Independent recommendation: accept the model implementation.
