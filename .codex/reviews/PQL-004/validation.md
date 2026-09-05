# PQL-004: Validation report

## Acceptance criteria

- PASS: All eight required types exist in the reusable `pql_domain` library.
- PASS: Private storage and explicit access prevent interchangeable primitives.
- PASS: Invalid numeric, identifier and symbol inputs return empty optionals.
- PASS: Compile-time assertions reject construction, implicit conversion and
  assignment between all different domain types, including Quantity/Symbol and
  PortfolioId/StrategyId.

## Commands and evidence

Implementer configured the existing cached dependencies without network access:

```powershell
cmake -S . -B build -G Ninja '-DCMAKE_MAKE_PROGRAM=C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe' -DCMAKE_BUILD_TYPE=Debug -DPQL_ENABLE_SANITIZERS=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build build
ctest --test-dir build --output-on-failure
& build/test/pql_domain_tests.exe
& build/src/pql_app.exe
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-format.exe' --dry-run --Werror src/domain/financial_types.hpp src/domain/financial_types.cpp test/test_financial_types.cpp
git -c safe.directory=C:/Users/geral/personal-quant-lab diff --check
```

MSVC Debug compilation passed with warnings treated as errors. CTest passed 2/2
registered tests: the existing three-case smoke suite and the new 14-case domain
suite. The domain executable independently reported 14/14 passed. The app ran and
printed its bootstrap message. Formatting and tracked diff whitespace checks passed.

Independent Validator also executed CTest (2/2 passed) and the domain executable
(14/14 passed). It reviewed malformed inputs, non-finite values, numeric bounds,
fractional/zero quantities, signed money, symbol ownership/moves, timestamps around
the epoch and the compile-time isolation matrix. No blocking defects were found.

## Failures investigated

The cached build initially referenced a sandbox-inaccessible WinGet Ninja launcher;
reconfiguring the local cache to the installed executable resolved it without a
repository build-policy change. The initial build then caught an overstrict test
assertion treating assignment to a returned chrono copy as mutation of storage.
The assertion was corrected to reject mutable reference access while allowing
independent value copies; the following build and tests passed.

## Regression risk and validation debt

Low regression risk: additive types and library with no existing financial consumers.
Linux, other compilers, Release mode and sanitizer execution remain **unverified**.
No persistence, API or live-data behavior is introduced or claimed as validated.

## Decision

**VALIDATION PASSED** — acceptance criteria satisfied on the tested configuration.
