# 005: Data quality

## What did I think before?

A starting assumption to challenge: if strategy code and portfolio formulas are
correct, the investment decisions they produce must be sensible.

## What is the concept?

Correct code can faithfully calculate the wrong answer from bad inputs. Prices
need identity, units, dates, consistent OHLC bounds and known availability, not
merely a successfully parsed HTTP response. Reject suspect data before it becomes
a decision input; never invent missing prices or silently repair inconsistent bars.

## Why does Personal Quant Lab need it?

Missing or negative prices corrupt fills and valuation. Duplicate dates can count
a session twice. Future information makes a historical strategy appear prescient.
Impossible OHLC relationships invalidate any calculation using those ranges.

Our daily adapter rejects the whole response on its first quality failure, before
filtering dates or opening a storage transaction. Quality diagnostics identify a
stable code, trusted field and validated session date without logging raw payloads
or credentials. Database and domain constraints provide additional protection.

## What failure would occur if we misunderstood it?

Silently replacing missing closes with zero could trigger false buy/sell signals.
Sorting an unordered JSON object is fine; silently choosing one of two conflicting
observations for the same date is not. An identical rerun of an accepted record is
idempotent, while duplicate dates inside one provider response reject—even when
the two values agree. Different values for an existing stored identity reject.

Rejecting dates after ingestion's captured UTC today is not protection against all
backtest look-ahead. A historical decision must only see information available at
its own decision time. Session dates do not encode the release time of closing data.

## What tiny example makes it intuitive?

With $1,000 cash, buying 2 shares at $100 should leave $800 before fees. If unchecked
input says the price is -$100, naive subtraction leaves $1,200: the purchase creates
money. Likewise, a bar with high=$90 and low=$110 implies a negative trading range;
`high < low` must reject rather than quietly swapping the values.

A real move from $100 to $110 is a 10% return. An erroneous $1,100 quote suggests a
1,000% return. All-positive, internally consistent OHLC values can still be wrong:
structural validation cannot independently establish that a vendor quote is true.

## What should I remember six months from now?

Perfect strategy code does not make imperfect data trustworthy. Validate at the
boundary, preserve provenance, report the reason for rejection, and test bad data
as deliberately as good data. Calendar gaps, stale prices, corporate actions,
cross-source reconciliation and decision-time availability need separate controls.
