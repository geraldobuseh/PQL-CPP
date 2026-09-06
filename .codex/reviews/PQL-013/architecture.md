# PQL-013 architecture review

Decision: ARCHITECTURE READY. Independent design/final review by architect.

A separate pql_market_data library implements the existing interface using owned
fixtures and pql_domain only. Known symbols come from bars; ordering and uniqueness
are validated per symbol without sorting. Latest is explicitly the final fixture
close. History has inclusive endpoints and distinguishes unknown symbols from an
empty known range. No adapter, persistence, clock or backtest framework was added.

Fixture assignment is disabled to keep validated histories intact; construction
from another provider copies its owned data. Partial construction cleans itself up
through ordinary container destruction. No remaining material design findings.
