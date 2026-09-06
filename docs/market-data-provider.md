# PQL-012: provider boundary

The engine consumes `pql::MarketDataProvider` from
`market_data/market_data_provider.hpp`. Its two pure virtual methods return a
`Price` or `std::vector<PriceBar>`; the destructor is virtual and defaulted. No
external SDK, SQL or HTTP types cross this interface. Application composition
chooses an adapter and passes a borrowed provider reference to a consumer.

`Date` is a validated Gregorian session-date label, years 1 through 9999. It is
neither a UTC timestamp nor proof of an open exchange session. `PriceBar` owns a
symbol, date, positive finite OHLC prices and nonnegative finite volume, including
fractional volume. Its factory rejects inconsistent OHLC bounds. These types follow
the existing optional-returning factories; fields cannot be individually mutated.

For v0.1 this interface means completed daily **raw USD-equity** bars. History uses
inclusive endpoints, ascending unique dates and the requested symbol only. Reversed
ranges throw `std::invalid_argument`. Unknown symbols, unavailable prices, malformed
responses and incomplete retrieval throw `MarketDataError`; adapters must not hide
these failures behind an empty result. A successful range with no observations can
be empty. Adapters must distinguish genuine nontrading sessions from missing data
using their source/calendar contract, without filling gaps or silently repairing
bad data. The abstract methods document these obligations; they do not implement
collection validation or an exchange calendar.

`getLatestPrice` does not carry a timestamp or guarantee freshness. Never use a live
latest-price call in a historical simulation. A bar's session date alone also does
not prove when its closing value was available. A future backtest boundary must
gate access by close/availability time. Raw bars are not a corporate-action-adjusted
or total-return series. Richer price/availability metadata is separate work.

The persistence `PriceObservation` remains a separate storage DTO with source,
timestamp and adjustment fields. A future adapter must explicitly map those facts
to this contract; reusing it here would couple engine inputs to persistence policy.

Synthetic tests exercise the exact signatures through a base reference, virtual
destruction, date validation, OHLC bounds and the documented query behavior. No
concrete vendor adapter, provider network call or engine orchestration exists in
this ticket. Adapter conformance will need its own tests when implemented.

Learning point: runtime polymorphism is justified here because the provider is a
real substitution boundary. The engine depends on the information it needs, and
composition owns the choice and lifetime of the object supplying it.
