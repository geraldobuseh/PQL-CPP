# PQL-005: Validation report

## Acceptance criteria and evidence

- PASS: Order and Trade exist as separate validated domain values.
- PASS: Only market-style simulated requests are constructible. There is no broker
  integration, limit/stop API, option model or short-selling mechanism.
- PASS: Buy and Sell creation tested independently with fractional quantities.
- PASS: Zero order quantity (both signs) rejected; negative and non-finite inputs
  rejected by Quantity before they can reach Order. Raw-double model construction
  is rejected at compile time. Trade inherits the validated positive order quantity.
- PASS: Order preserves the requested symbol, side, quantity and timestamp, plus
  supplied OrderId, Market type and Pending status.

Short-selling prevention at execution requires holdings context and is not claimed
as implemented. The models do not execute trades. This boundary was reviewed by
Architecture and Quant and is documented in code and the ticket contract.

## Actual commands and results

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
& build/test/pql_order_trade_tests.exe
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-format.exe' --dry-run --Werror src/domain/order.hpp src/domain/order.cpp src/domain/trade.hpp src/domain/trade.cpp test/test_order_trade.cpp
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

MSVC Debug build passed with warnings treated as errors. All 3 CTest registrations
passed: smoke, primitive types and order/trade. The new executable reported 15/15
tests passed. Changed C++ formatting and tracked diff whitespace checks passed.

The independent Validator also ran CTest (3/3 passed) and the order/trade executable
(15/15 passed), and inspected implementation, tests, CMake diff and design/quant
contracts. No blocking defects were found.

## Edge cases and regression risk

Coverage includes invalid side casts, fractional and extreme finite quantities,
zero and invalid primitive inputs, copied/moved source ownership, complete field
preservation, equal and later execution times, rejection before submission and
explicit pre-epoch instants. Trade creation leaves the Order request unchanged.

The change is additive to PQL-004 and introduces no portfolio/accounting mutation.
Earlier uncommitted PQL-004 work is preserved.

## Remaining validation debt

Sanitizers, Release and other compilers/platforms remain **unverified**. This ticket
does not implement or validate execution authorization, holdings/cash checks,
duplicate-fill prevention, status transitions, costs or persistence.

## Decision

**VALIDATION PASSED** for the model scope on the tested configuration.
