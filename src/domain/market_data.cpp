#include "domain/market_data.hpp"

namespace pql {

std::optional<Date> Date::create(int year, unsigned month, unsigned day) {
    // Check before chrono's narrow component constructors can wrap large inputs.
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31) {
        return std::nullopt;
    }
    const std::chrono::year_month_day value{
        std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
    if (!value.ok()) return std::nullopt;
    return Date{value};
}

PriceBar::PriceBar(const Symbol& symbol, Date date, Price open, Price high, Price low,
                   Price close, Quantity volume)
    : symbol_(symbol), date_(date), open_(open), high_(high), low_(low),
      close_(close), volume_(volume) {}

std::optional<PriceBar> PriceBar::create(const Symbol& symbol, Date date, Price open,
                                       Price high, Price low, Price close, Quantity volume) {
    if (low.value() > high.value() || open.value() < low.value() ||
        open.value() > high.value() || close.value() < low.value() || close.value() > high.value()) {
        return std::nullopt;
    }
    return PriceBar{symbol, date, open, high, low, close, volume};
}

}  // namespace pql
