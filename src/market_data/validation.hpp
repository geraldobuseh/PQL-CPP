#pragma once

#include "market_data/market_data_provider.hpp"

namespace pql {

enum class MarketDataIssue {
    MissingPrice,
    InvalidPrice,
    DuplicateDate,
    FutureTimestamp,
    InvalidOhlc,
    InvalidVolume,
    InvalidDate,
    MalformedResponse
};
enum class MarketDataField { None, Open, High, Low, Close, Volume, SessionDate, LastRefreshed };

[[nodiscard]] inline const char* issueName(MarketDataIssue issue) noexcept {
    switch (issue) {
        case MarketDataIssue::MissingPrice:
            return "missing_price";
        case MarketDataIssue::InvalidPrice:
            return "invalid_price";
        case MarketDataIssue::DuplicateDate:
            return "duplicate_date";
        case MarketDataIssue::FutureTimestamp:
            return "future_timestamp";
        case MarketDataIssue::InvalidOhlc:
            return "invalid_ohlc";
        case MarketDataIssue::InvalidVolume:
            return "invalid_volume";
        case MarketDataIssue::InvalidDate:
            return "invalid_date";
        case MarketDataIssue::MalformedResponse:
            return "malformed_response";
    }
    return "unknown_validation_issue";
}
[[nodiscard]] inline const char* fieldName(MarketDataField field) noexcept {
    switch (field) {
        case MarketDataField::None:
            return "response";
        case MarketDataField::Open:
            return "open";
        case MarketDataField::High:
            return "high";
        case MarketDataField::Low:
            return "low";
        case MarketDataField::Close:
            return "close";
        case MarketDataField::Volume:
            return "volume";
        case MarketDataField::SessionDate:
            return "session_date";
        case MarketDataField::LastRefreshed:
            return "last_refreshed";
    }
    return "unknown_field";
}

// Safe diagnostic context: only enums and a validated date, never raw payloads,
// API keys, arbitrary JSON keys or untrusted numeric strings. Fail on first issue.
class MarketDataValidationError : public MarketDataError {
   public:
    explicit MarketDataValidationError(MarketDataIssue issue,
                                       MarketDataField field = MarketDataField::None,
                                       std::optional<Date> date = std::nullopt)
        : MarketDataError(issueName(issue)), issue_(issue), field_(field), date_(date) {}
    [[nodiscard]] MarketDataIssue issue() const noexcept { return issue_; }
    [[nodiscard]] MarketDataField field() const noexcept { return field_; }
    [[nodiscard]] std::optional<Date> date() const noexcept { return date_; }

   private:
    MarketDataIssue issue_;
    MarketDataField field_;
    std::optional<Date> date_;
};

}  // namespace pql
