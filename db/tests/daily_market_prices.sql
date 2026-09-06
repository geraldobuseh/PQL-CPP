\set ON_ERROR_STOP on
BEGIN;
CREATE FUNCTION pg_temp.rejects(statement text, expected_state text) RETURNS void LANGUAGE plpgsql AS $$
BEGIN
    BEGIN
        EXECUTE statement;
    EXCEPTION WHEN OTHERS THEN
        IF SQLSTATE=expected_state THEN RETURN; END IF;
        RAISE;
    END;
    RAISE EXCEPTION 'Unexpected successful statement';
END;
$$;
INSERT INTO assets(asset_id,symbol,name,currency) VALUES(900014,'DAILYTEST','Daily tests','USD');
INSERT INTO market_prices(asset_id,observed_at,source,adjustment,open,high,low,close,volume,session_date)
VALUES(900014,'2026-01-02 00:00:00+00','alphavantage.daily','raw',100,110,90,105,1000,'2026-01-02');
SELECT pg_temp.rejects($s$UPDATE market_prices SET observed_at='2026-01-02 12:00:00+00' WHERE asset_id=900014$s$,'23514');
SELECT pg_temp.rejects($s$UPDATE market_prices SET session_date=NULL WHERE asset_id=900014$s$,'23514');
SELECT pg_temp.rejects($s$UPDATE market_prices SET adjustment='split_adjusted' WHERE asset_id=900014$s$,'23514');
SELECT pg_temp.rejects($s$UPDATE market_prices SET session_date='infinity' WHERE asset_id=900014$s$,'23514');
SELECT pg_temp.rejects($s$INSERT INTO market_prices SELECT * FROM market_prices WHERE asset_id=900014$s$,'23505');
ROLLBACK;
\echo 'Daily schema tests passed'
