# PQL-006: C++20 review

## Independent review and decision

**C++ REVIEW PASSED** — no material implementation findings. The reviewer inspected
Position, its tests, ticket contract, learning note and design/quant reviews.

## Ownership, lifetime and exception safety

Position owns its Symbol and scalar state. Updates calculate local candidates and
construct a new Position only after validation; rejection cannot mutate the source.
Symbol's established copy behavior preserves validity across generated moves.
Allocation failure does not corrupt an existing position.

All optional arithmetic results are checked before use. The only unconditional
factory dereferences are guaranteed-valid zero constants in empty(). No raw
ownership, unsafe lifetime assumptions, signed-integer arithmetic or undefined
behavior were identified.

## Invariants and numerical boundaries

Finite-value construction, positive-value product checks, erased-contribution
rejection, exact oversell/liquidation rules and remaining-basis checks preserve the
documented state. Average cost is unchanged on partial sales; liquidation clears
it and retains realized history. Timestamp ordering is checked before updates.

## Resolved finding and validation scope

LOW wording: the original header comment could imply realized P&L resets when flat.
It now explicitly names unrealized P&L and states realized history remains.

This review was static. Root/Validator execution evidence, including the additional
realized-overflow and underflow tests, is in `validation.md`.
