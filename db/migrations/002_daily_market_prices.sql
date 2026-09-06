-- Explicit session labels for daily ingestion; existing timestamp observations remain valid.
ALTER TABLE market_prices ADD COLUMN session_date date;
ALTER TABLE market_prices ADD CONSTRAINT daily_session_consistent CHECK (
    session_date IS NULL OR (
        session_date BETWEEN DATE '0001-01-01' AND DATE '9999-12-31'
        AND observed_at = (session_date::timestamp AT TIME ZONE 'UTC')
    )
);
ALTER TABLE market_prices ADD CONSTRAINT alphavantage_daily_contract CHECK (
    source <> 'alphavantage.daily' OR (session_date IS NOT NULL AND adjustment = 'raw')
);
CREATE UNIQUE INDEX market_prices_daily_identity ON market_prices(asset_id,session_date,source,adjustment)
    WHERE session_date IS NOT NULL;
COMMENT ON COLUMN market_prices.session_date IS 'Daily session label; corresponding UTC midnight is a key, not availability or exchange-close time. NULL for legacy timestamp observations.';
