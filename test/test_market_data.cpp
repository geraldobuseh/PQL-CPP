#include <gtest/gtest.h>

#include <limits>
#include <memory>
#include <type_traits>

#include "market_data/market_data_provider.hpp"

namespace {
using namespace pql;

static_assert(std::is_abstract_v<MarketDataProvider>);
static_assert(std::has_virtual_destructor_v<MarketDataProvider>);
static_assert(std::is_same_v<decltype(&MarketDataProvider::getLatestPrice),
                             Price (MarketDataProvider::*)(const Symbol&)>);
static_assert(std::is_same_v<decltype(&MarketDataProvider::getHistory),
                             std::vector<PriceBar> (MarketDataProvider::*)(const Symbol&, Date, Date)>);
static_assert(!std::is_default_constructible_v<Date> && !std::is_aggregate_v<Date>);
static_assert(!std::is_default_constructible_v<PriceBar> && !std::is_aggregate_v<PriceBar>);
static_assert(!std::is_convertible_v<Timestamp, Date>);

Date date(int year, unsigned month, unsigned day) { return *Date::create(year, month, day); }
const Symbol spy = *Symbol::create("SPY");
Price price(double value) { return *Price::create(value); }
PriceBar bar(Date day, double close) {
    return *PriceBar::create(spy, day, price(close), price(close), price(close),
                             price(close), *Quantity::create(0.5));
}

// Test-only implementation. Engine-facing consumer below knows only the abstract
// interface; this fixture intentionally requires no PostgreSQL or network client.
class SyntheticProvider final : public MarketDataProvider {
   public:
    explicit SyntheticProvider(double latest, bool& destroyed)
        : latest_(price(latest)), destroyed_(destroyed) {}
    ~SyntheticProvider() override { destroyed_ = true; }
    Price getLatestPrice(const Symbol& symbol) override {
        if (symbol != spy) throw MarketDataError("Unknown synthetic symbol");
        return latest_;
    }
    std::vector<PriceBar> getHistory(const Symbol& symbol, Date from, Date to) override {
        if (from > to) throw std::invalid_argument("Reversed history range");
        if (symbol != spy) throw MarketDataError("Unknown synthetic symbol");
        std::vector<PriceBar> result;
        for (const auto& item : bars_) {
            if (item.date() >= from && item.date() <= to) result.push_back(item);
        }
        return result;
    }
   private:
    Price latest_;
    bool& destroyed_;
    const std::vector<PriceBar> bars_{bar(date(2026, 1, 2), 100), bar(date(2026, 1, 5), 110)};
};

Price consumeLatest(MarketDataProvider& provider, const Symbol& symbol) {
    return provider.getLatestPrice(symbol);
}

TEST(MarketData, DatesValidateCalendarAndNarrowingBoundaries) {
    EXPECT_TRUE(Date::create(2024, 2, 29));
    EXPECT_TRUE(Date::create(2000, 2, 29));
    EXPECT_FALSE(Date::create(1900, 2, 29));
    EXPECT_FALSE(Date::create(2025, 2, 29));
    EXPECT_FALSE(Date::create(2026, 4, 31));
    EXPECT_TRUE(Date::create(1, 1, 1));
    EXPECT_TRUE(Date::create(9999, 12, 31));
    for (int year : {0, -1, 10000, std::numeric_limits<int>::max()}) {
        EXPECT_FALSE(Date::create(year, 1, 1));
    }
    for (unsigned month : {0U, 13U, 257U, std::numeric_limits<unsigned>::max()}) {
        EXPECT_FALSE(Date::create(2026, month, 1));
    }
    for (unsigned day : {0U, 32U, 257U, std::numeric_limits<unsigned>::max()}) {
        EXPECT_FALSE(Date::create(2026, 1, day));
    }
    EXPECT_LT(date(2025, 12, 31), date(2026, 1, 1));
    EXPECT_EQ(date(2024, 2, 29).value(), (std::chrono::year{2024}/2/29));
}

TEST(MarketData, BarsPreserveFieldsAndAllowZeroOrFractionalVolume) {
    for (double volume : {0.0, 0.125}) {
        const auto item = PriceBar::create(spy, date(2026, 1, 2), price(100), price(120),
                                          price(90), price(110), *Quantity::create(volume));
        ASSERT_TRUE(item);
        EXPECT_EQ(item->symbol(), spy);
        EXPECT_EQ(item->date(), date(2026, 1, 2));
        EXPECT_EQ(item->open(), price(100));
        EXPECT_EQ(item->high(), price(120));
        EXPECT_EQ(item->low(), price(90));
        EXPECT_EQ(item->close(), price(110));
        EXPECT_EQ(item->volume(), *Quantity::create(volume));
    }
    EXPECT_TRUE(PriceBar::create(spy, date(2026, 1, 2), price(1), price(1), price(1),
                               price(1), *Quantity::create(0)));
}

TEST(MarketData, RejectsEachInvalidOhlcRelationship) {
    const auto day = date(2026, 1, 2);
    const auto volume = *Quantity::create(1);
    EXPECT_FALSE(PriceBar::create(spy, day, price(100), price(120), price(110), price(115), volume));
    EXPECT_FALSE(PriceBar::create(spy, day, price(130), price(120), price(90), price(110), volume));
    EXPECT_FALSE(PriceBar::create(spy, day, price(100), price(120), price(90), price(80), volume));
    EXPECT_FALSE(PriceBar::create(spy, day, price(100), price(120), price(90), price(130), volume));
    EXPECT_FALSE(PriceBar::create(spy, day, price(100), price(90), price(110), price(100), volume));
}

TEST(MarketData, ConsumerUsesInterfaceAndVirtualDestruction) {
    bool first_destroyed = false;
    bool second_destroyed = false;
    {
        std::unique_ptr<MarketDataProvider> first = std::make_unique<SyntheticProvider>(100, first_destroyed);
        std::unique_ptr<MarketDataProvider> second = std::make_unique<SyntheticProvider>(200, second_destroyed);
        EXPECT_EQ(consumeLatest(*first, spy), price(100));
        EXPECT_EQ(consumeLatest(*second, spy), price(200));
    }
    EXPECT_TRUE(first_destroyed);
    EXPECT_TRUE(second_destroyed);
}

TEST(MarketData, HistoryThroughInterfaceUsesInclusiveDatesAndReportsErrors) {
    bool destroyed = false;
    SyntheticProvider fixture(110, destroyed);
    MarketDataProvider& provider = fixture;
    const auto from = date(2026, 1, 2);
    const auto to = date(2026, 1, 5);
    const auto history = provider.getHistory(spy, from, to);
    ASSERT_EQ(history.size(), 2U);
    EXPECT_EQ(history.front(), bar(from, 100));
    EXPECT_EQ(history.back(), bar(to, 110));
    EXPECT_EQ(provider.getHistory(spy, from, from), (std::vector<PriceBar>{bar(from, 100)}));
    EXPECT_TRUE(provider.getHistory(spy, date(2026, 1, 3), date(2026, 1, 4)).empty());
    EXPECT_THROW((void)provider.getHistory(spy, to, from), std::invalid_argument);
    const auto unknown = *Symbol::create("UNKNOWN");
    EXPECT_THROW((void)provider.getLatestPrice(unknown), MarketDataError);
    EXPECT_THROW((void)provider.getHistory(unknown, from, to), MarketDataError);
}
}  // namespace
