# PQL-008: C++20 review

## Independent decision

**C++ REVIEW PASSED** — no material findings after inspection of Transaction,
Portfolio, Position, migrated/new tests, ticket contract and learning note.

## Immutability, ownership and containers

Transaction owns its PortfolioId, Trade and Money fees. It is copy-constructible but
nonassignable and exposes no mutable fields. Rvalue construction copies without
invalidating the source. The history uses only vector copy construction, append and
swap, all compatible with nonassignable records. No element assignment is introduced.

## Exception safety and accounting boundary

Snapshot's existing copy-before-swap avoids history element assignment and preserves
strong exception safety. applyTransaction validates portfolio/identity before staging;
all allocating work occurs on candidate state and final swaps cannot throw. Replay
and the zero-fee compatibility adapter invoke the sole authoritative path.

Position's typed fee overload keeps portfolio ownership out of symbol-level accounting.
Fee arithmetic rejects invalid signs, non-finite values and erased contributions,
while permitting legitimate zero or negative net proceeds and exact P&L cancellation.
Strategy snapshots expose neither trade nor transaction application.

## Validation scope

Static review only; no independent runtime or allocation-fault injection was performed
by the C++ Reviewer. Executed root/Validator evidence is in validation.md.
