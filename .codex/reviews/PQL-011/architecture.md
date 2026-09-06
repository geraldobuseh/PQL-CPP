# PQL-011 architecture review

Decision: ARCHITECTURE READY; final review PASSED.

Architect independently reviewed design and implementation; root owns code and
runtime validation. The optional pql_persistence target leaves domain code free
of database dependencies. Three repository interfaces expose domain values and
typed metadata, while one noncopyable PostgresUnitOfWork owns connection/transaction
lifetimes and commits coordinated writes explicitly.

Append requires the original Order to retain submission time. A portfolio row lock
precedes replay, domain validation and sequence allocation; matching order/execution/
transaction rows commit together. Reads reconstruct state rather than trusting
projection tables. A portfolio name update meets the legitimate-update criterion;
market correction is not part of the ticket's acceptance criteria.

The canonical decimal protocol rejects precision loss. Timestamp conversion uses
integer milliseconds. Not-found differs from failure, and caught operation errors
cannot permit a partial commit. SQL remains in the adapter. No pool, ORM, schema
change or strategy mutation API was introduced.

Resolved MEDIUM: embedded NUL truncation in bound text, now rejected before binding
or connection construction. Reviewed regression covers lookup aliasing and rollback.
No unresolved material findings. Native Windows linking and uncertain-commit fault
injection remain explicit validation debt.
