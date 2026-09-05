# PQL-007: C++20 review

## Independent decision

**C++ REVIEW PASSED** — no material findings after static inspection of source,
tests, ticket contract and architecture/quant reviews.

## Ownership and lifetime

Snapshot owns detached vectors and has no authority back-reference or trade mutation
API. Declared copy operations suppress moves that could empty vectors while leaving
cash inconsistent. Generated Portfolio moves copy its Snapshot member and leave the
source coherent. Tests exercise independent copies, assignments and source lifetimes.

## Exception safety and invariants

All allocating work in applyTrade occurs on local candidate data. The final state
swap only exchanges scalar domain values and default-allocator vectors, so it cannot
throw. Snapshot assignment copies its argument before entering the noexcept swap
body; copying failure leaves the destination unchanged.

The original position iterator remains valid while separate candidate containers
are changed, and its index addresses the equivalent copied position order. Mark
iteration evaluates first+1 only for a non-end iterator. Checked optional results,
cash sufficiency and Position validation compose correctly.

No undefined behavior, unsafe ownership or lifetime defect was identified. No
performance improvement is claimed; candidate copying is a deliberate correctness
choice for the current small portfolio scope.

## Validation scope

This was a static review. Allocation-failure injection was not performed. Executed
tests, including the final additional cash-credit regression, are in `validation.md`.
