# PQL-013 quantitative review

Decision: QUANT REVIEW PASSED. Independent static design and final review by quant.

The fake returns known immutable fixture prices, rejects duplicate/decreasing dates,
and has no time progression. Historical simulation uses exact-date history rather
than final-fixture latest prices. Decisions and synthetic execution steps are fixed;
this test makes no claim about historical information availability or profitability.

Verified oracle: starting1000; buy2@100 fee2 ->cash798/avg101; buy2@120 fee2
->cash556/avg111; sell1@130 fee1 ->cash685/shares3/realized18; sell3@90 fee3
->cash952/shares0/realized-48. Intermediate unrealized P&L is -2,36,57,0;
total value is998,1036,1075,952. Repeated simulation and ledger replay are tested.
No financial findings. Exchange calendars and vendor data quality remain out of scope.
