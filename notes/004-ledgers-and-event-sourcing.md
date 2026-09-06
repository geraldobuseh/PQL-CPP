# 004: Ledgers and event sourcing

## What did I think before?

A starting assumption to question is: "If I store the latest cash balance and share
count, I have stored the portfolio." Those numbers describe the present but do not
explain which purchases, sales and fees produced it.

## What is the concept?

A ledger records financial facts. In this scope each **Transaction** is an immutable
full-fill event containing portfolio ID, originating order ID, symbol, side, quantity,
execution price, explicit fees and execution timestamp.

**Event sourcing** means deriving current state by applying those events in order to
a known starting state. Cash and Position values are projections of the history.
The engine retains starting cash separately; it is not an invented buy transaction.

```text
portfolio ID + starting cash + ordered transactions -> cash and positions
cash and positions + supplied current market prices -> portfolio value
```

Records can be copied but cannot be assigned to or changed through the public API.
Accepted history is exposed read-only. Cash, positions and history commit together;
rejected transactions change none of them. The application rejects wrong-portfolio
events, duplicate order IDs, decreasing times, oversells and unaffordable debits.
Equal-time events preserve their recorded sequence.

## Why does Personal Quant Lab need it?

The history explains why the investor owns a share and why cash moved. We can replay
it to reproduce the same accounting, inspect a disputed fee, compare a projection
against the events, or rebuild derived state after a calculation defect is fixed.
A rebuilt projection using changed accounting rules must be identified as such;
unchanged inputs and rules give deterministic results.

Strategies still receive a detached PortfolioSnapshot. They propose Orders and do
not append financial records. Trusted execution supplies Transactions; Portfolio
validates and applies them through the same path used for replay.

## What failure would occur if we misunderstood it?

Overwriting a $1,000 balance with $920 cannot establish whether $80 was invested,
withdrawn or lost to an incorrect calculation. A separate mutable balance and mutable
history can disagree if a failure occurs between writes. Editing an old price or fee
also destroys the evidence needed to explain previously reported results.

Storing immutable events is safer because it preserves that evidence and makes the
balance reproducible. It does not make bad input correct or storage durable by itself.
Future persistence needs transactional writes, access control and backups. Corrections
need explicit recorded semantics rather than silently editing accepted events; a
correction/reversal API is not implemented in this ticket.

## What tiny example makes it intuitive?

Start with **$1,000**, no holdings, and explicitly **zero fees**:

| Event | Cash | AAPL shares | Average cost | Cumulative realized profit |
| --- | ---: | ---: | ---: | ---: |
| Initial state | $1,000 | 0 | Absent | $0 |
| BUY 2 AAPL at $100 | $800 | 2 | $100 | $0 |
| SELL 1 AAPL at $120 | $920 | 1 | $100 | $20 |

The purchase debits `2 × $100 = $200`; the sale credits `$120`. Realized profit is
`1 × ($120 − $100) = $20`. At an **explicitly supplied $120 market mark**, the remaining
share is worth $120, giving total value `$920 + $120 = $1,040`. The other $20 gain is
unrealized. A sale price in history is not automatically a current market quote;
missing current marks cause valuation to fail rather than guess.

Now give the buy a **$2 fee** and the sale a **$1 fee**:

- Purchase cash becomes `$1,000 − $200 − $2 = $798`.
- Acquisition basis is `$202 / 2 = $101` per share because buy fees are capitalized.
- Sale cash becomes `$798 + $120 − $1 = $917`.
- Realized profit is `$120 − $101 − $1 = $18`.
- At the same $120 mark, unrealized profit is `$120 − $101 = $19` and total value
  is `$917 + $120 = $1,037`.

The $37 gain reconciles as `$18 realized + $19 unrealized`, or `$40 gross gain − $3
fees`. Replay must read the recorded fees, not assume zero or look up a new fee schedule.
The current weighted-average convention is research accounting, not tax-lot reporting.

## What should I remember six months from now?

Store the facts needed to explain a balance; treat the balance as derived state.
Keep original events, starting state, their order and the accounting policy explicit.
Current valuation additionally needs a market mark and an appropriate valuation time.

Buy fees increase basis. Sell fees reduce realized profit and cash proceeds. A sale
whose fees equal its proceeds still removes shares even though cash does not move;
fees above proceeds consume existing cash and must not create a negative balance.

This implementation stores its ledger in memory. Durable database persistence,
deposits/withdrawals, corrections, corporate actions and accounting-version metadata
remain future work. It uses approximate doubles with explicit arithmetic rejection,
not decimal settlement amounts. Immutability here is a C++ API guarantee, not proof
against malicious memory writes or a substitute for durable storage controls.
