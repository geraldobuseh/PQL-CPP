# PQL-005: C++20 review

## Independent review scope

Reviewed order/trade headers and sources, tests, CMake wiring, ticket contract and
architecture/quant artifacts. This was a static review; executed test evidence is
recorded separately in `validation.md`.

## Ownership, lifetime and safety

Order and Trade own all stored data. They retain no reference to the source Order
or input Symbol. Generated moves preserve validity because Symbol intentionally
copies from rvalues. No raw ownership, unchecked optional dereferences in production,
unsafe lifetime assumptions or undefined behavior were identified.

## Domain API

Primitive invariants compose with the additional positive-quantity and valid-side
checks. A Trade derives identity, symbol, side and full quantity from Order, removing
the possibility of independently supplied mismatched fields. Timestamp comparison
adds no arithmetic or overflow. Construction does not mutate Order.

Pending-only creation and full-fill record scope are documented. Execution, lifecycle
and portfolio-dependent validation are explicitly outside these models.

## Build and tests

Both sources are registered with `pql_domain`; test linkage inherits its public
include path and C++20 requirement. Tests cover the typed construction boundary,
ownership, both sides, quantity rejection and time consistency.

## Findings and decision

No material findings. **C++ REVIEW PASSED**.
