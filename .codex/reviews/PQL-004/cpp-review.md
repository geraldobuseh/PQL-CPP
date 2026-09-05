# PQL-004: C++20 review

## Ownership, lifetime and exception safety

Independent C++ Reviewer inspected implementation, tests, CMake and contract docs.
Symbol owns its string; explicitly defaulted copy operations suppress implicit
moves and deliberately preserve source validity. Private storage and restricted
construction enforce domain separation. Standard RAII covers allocation failures.
No owning raw pointers, dangling references or lifetime defects were identified.

## Undefined behavior and API clarity

ASCII validation avoids locale dependence and signed-character classification
hazards. Numeric factories reject non-finite values and enforce documented sign
constraints. No internal financial arithmetic or overflow is introduced. Timestamp
construction and comparisons are deterministic. Primitive conversions before entry
to factories remain documented responsibilities of future input boundaries.

CMake propagates C++20 and include paths through the domain library target. The
compile-time test matrix covers every ordered pair of domain types.

## Findings and decision

No material findings. **C++ REVIEW PASSED**.

This was a static independent review; the C++ Reviewer did not execute tests.
Executed build/test evidence is recorded in `validation.md`.
