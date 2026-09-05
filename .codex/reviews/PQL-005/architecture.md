# PQL-005: Architecture review

## Objective and current surface

Add Order and Trade models to the existing PQL-004 `pql_domain` library. There is
no broker, portfolio, ledger, API or schema to extend. Preserve private value storage,
typed inputs, optional validation results and deterministic behavior.

## Independent Architecture Agent findings

Use separate order/trade headers and implementation files. Order needs OrderId to
link a Trade, plus the requested symbol, side, quantity, type, timestamp and status.
Only Buy/Sell and Market are supported; creation starts Pending. Reject cast side
values and zero quantities (the primitive Quantity intentionally permits zero).

Trade records originating OrderId, symbol, side, executed quantity, price and time.
Execution, lifecycle operations and portfolio mutation are outside scope. Sell
requests cannot enforce a holdings-dependent no-short-selling rule without an
execution/portfolio boundary; do not invent a holdings argument in model factories.

## Final implementation contract

Order exposes only `create_market`, always supplying Market/Pending internally.
Trade derives request fields and full positive quantity from an Order and rejects
execution timestamps before submission. It does not execute, mutate the request or
prevent repeated records. No unsupported type/status can be supplied to a factory.
Fees, portfolio attribution, lifecycle states beyond Pending and partial fills are
deferred; the Quant Reviewer explicitly approved this bounded request/record scope.

## Validation requirements and decision

Test both sides, zero rejection, malformed sides, fractional quantities, initial
status, typed construction boundaries, independent ownership and field preservation.
Document future execution responsibilities and avoid financial arithmetic.

**ARCHITECTURE READY** — small additive models, no execution-engine expansion.
