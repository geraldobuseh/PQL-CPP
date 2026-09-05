# PQL-004: Architecture review

## Objective and current architecture

Create eight distinct financial primitives in the existing C++20 bootstrap. There
are no domain callers, database schemas or financial fields to migrate.

## Design and contracts

Independent Architecture Agent recommended a small `pql_domain` library, separate
classes with private storage, read-only access, same-type equality, optional
factories for validated inputs and an explicit chrono timestamp constructor.
Implementation places the header in `src/domain/financial_types.hpp`, keeping the
small library beside the existing sources rather than adding an installable SDK.

No implicit conversions, public default construction, arithmetic, ID allocation,
clock access or persistence are introduced. Quant review supplies numeric policies.
Symbol preserves its invariant after moves by copying its owned string.

## Risks and validation requirements

Document approximate floating-point values, currency assumptions, quantity/price
semantics, timestamp precision and future primitive parsing responsibilities.
Test every ordered pair of domain types for construction/assignment isolation;
exercise non-finite values, numeric boundaries, malformed symbols and timestamps.

## Decision

**ARCHITECTURE READY** — smallest coherent domain library, no speculative framework.
The README ticket roadmap mismatch is an adjacent documentation issue.
