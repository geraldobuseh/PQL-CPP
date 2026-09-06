# PQL-015: Quantitative Correctness Review

QUANT REVIEW PASSED. Recorded from quant agent's final static review; no financial
findings. The reviewer did not rerun tests.

Missing/invalid OHLC, impossible bounds, duplicate dates, future dates and invalid
volume/calendar dates fail before filtering or storage. Deterministic validation
errors do not retry. Invalid excluded rows and partial persistence are tested.

The learning note's examples are correct: buying two shares at100 from1000 leaves
800, while accepting negative100 would incorrectly yield1200. A100-to110 move is
10%; an erroneous1100 quote implies1000%. Ingestion future-date checks explicitly
do not establish historical decision-time information availability. Structural
validation alone cannot prove economic accuracy. No benchmark calculation changes.
