# 003: Cost basis, realized P&L and unrealized P&L

## What did I think before?

A useful starting intuition to challenge is: "My purchase price is my cost, and a
higher market price means I have made that profit." Multiple purchases need a
weighted average, and a price gain on shares still held is different from a gain
on shares already sold.

## What is the concept?

This project uses **weighted-average cost** for research accounting in one currency.
It does not select tax lots. In the current model, trades have no fee fields, so the
examples and calculations are before fees and other costs.

- **Cost basis** is the acquisition cost allocated to the shares still owned:
  `shares × average cost per share`.
- **Realized P&L** comes from shares sold:
  `shares sold × (sale price − average cost)`.
- **Market value** is what the remaining shares are worth at the supplied mark:
  `shares held × market price`.
- **Unrealized P&L** is the gain or loss on those remaining shares:
  `shares held × (market price − average cost)`.

On a buy, the new average is:

```text
(old shares × old average + bought shares × fill price)
-----------------------------------------------------
                  old shares + bought shares
```

A partial sale removes basis for the shares sold and leaves the per-share average
unchanged. Complete liquidation leaves zero shares and zero remaining basis; average
cost is absent because there are no shares to average. Realized history remains.

## Why does Personal Quant Lab need it?

Position reconstructs these quantities from supplied Trade records. Each accepted
trade returns a new Position; it cannot directly change cash or another position.
A market mark only affects valuation and unrealized P&L. It does not rewrite cost
basis or realized P&L.

This separation lets the future portfolio layer distinguish money invested, results
of completed sales, and gains or losses still exposed to market prices. These are
accounting amounts, not a benchmark return or proof of investment skill.

## What failure would occur if we misunderstood it?

Using an unweighted average misprices unequal purchases: 10 shares at $100 plus
30 at $120 cost $4,600. The average is $115, not $110.

Reducing average cost by sale proceeds would mix realized profit with the basis of
remaining shares. Treating unrealized profit as cash could fund a purchase with
money the investor has not actually received. Forgetting to clear the average after
liquidation would contaminate the next purchase with an old position's cost.

## What tiny example makes it intuitive?

Use a hypothetical SPY mark of **$125** throughout this table. It is an explicit
valuation input, not a fetched live price or the price of every trade.

| Event | Shares held | Average cost | Remaining basis | Market value | Unrealized P&L | Cumulative realized P&L |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Start empty | 0 | Absent | $0 | $0 | $0 | $0 |
| Buy 10 at $100 | 10 | $100 | $1,000 | $1,250 | $250 | $0 |
| Buy 10 at $120 | 20 | $110 | $2,200 | $2,500 | $300 | $0 |
| Sell 5 at $130 | 15 | $110 | $1,650 | $1,875 | $225 | $100 |
| Sell remaining 15 at $90 | 0 | Absent | $0 | $0 | $0 | −$200 |

The first sale realizes `5 × ($130 − $110) = $100`. The final sale realizes
`15 × ($90 − $110) = −$300`, so cumulative realized P&L is `$100 − $300 = −$200`.
As a cross-check, total sale proceeds are `$650 + $1,350 = $2,000`, versus $2,200
spent buying. After liquidation, the entire $200 loss is realized.

If the investor then buys 2 shares at $50, the new position has a $50 average and
$100 basis. The previous −$200 realized result remains recorded.

## What should I remember six months from now?

Buys change weighted average cost; partial sells preserve it; liquidation clears it.
Realized P&L belongs to sold shares, unrealized P&L to shares still held. Changing a
mark does not change how much those shares originally cost.

The implementation uses approximate `double` values, with no cent rounding or
epsilon-based oversell forgiveness. For example, the represented remainder after
`0.3 − 0.1` can be slightly below `0.2`. Sell the reported remaining quantity to
liquidate exactly; do not silently clamp an inconsistent sell request. Arithmetic
overflow, value underflow and swallowed quantity adjustments return rejection.

The caller must supply unique fills in nondecreasing execution-time order and a
suitable market mark. Equal-time fills retain caller order. Position does not fetch
prices, track cash, deduplicate fills or persist the financial source of truth.
Fees, corporate actions, multi-currency handling, exact decimal arithmetic and
tax-lot accounting need separate designs; these examples are not tax reporting.
