#pragma once

#include "domain/financial_types.hpp"

namespace pql {

// Gregorian session-date label, years 1..9999. Not a timestamp or a guarantee
// that an exchange traded on this date; calendars belong to provider adapters.
class Date {
   public:
    [[nodiscard]] static std::optional<Date> create(int year, unsigned month, unsigned day);
    [[nodiscard]] std::chrono::year_month_day value() const noexcept { return value_; }
    auto operator<=>(const Date&) const = default;

   private:
    explicit Date(std::chrono::year_month_day value) : value_(value) {}
    std::chrono::year_month_day value_;
};

// Completed daily, raw USD-equity OHLCV bar. No adjusted-price mixing.
// Validated construction preserves OHLC bounds throughout the value's lifetime.
class PriceBar {
   public:
    [[nodiscard]] static std::optional<PriceBar> create(const Symbol& symbol, Date date,
        Price open, Price high, Price low, Price close, Quantity volume);
    [[nodiscard]] const Symbol& symbol() const noexcept { return symbol_; }
    [[nodiscard]] Date date() const noexcept { return date_; }
    [[nodiscard]] Price open() const noexcept { return open_; }
    [[nodiscard]] Price high() const noexcept { return high_; }
    [[nodiscard]] Price low() const noexcept { return low_; }
    [[nodiscard]] Price close() const noexcept { return close_; }
    [[nodiscard]] Quantity volume() const noexcept { return volume_; }
    bool operator==(const PriceBar&) const = default;

   private:
    PriceBar(const Symbol& symbol, Date date, Price open, Price high, Price low,
             Price close, Quantity volume);
    Symbol symbol_;
    Date date_;
    Price open_;
    Price high_;
    Price low_;
    Price close_;
    Quantity volume_;
};

}  // namespace pql
