#include <gtest/gtest.h>

#include "alpha_fixture.hpp"
#include "market_data/validation.hpp"

namespace {
using namespace pql;
const auto aapl = *Symbol::create("AAPL");
const auto today = *Date::create(2026, 1, 6);
const auto from = *Date::create(2026, 1, 2);
const auto to = *Date::create(2026, 1, 5);

void expectIssue(const std::string& payload, MarketDataIssue issue, MarketDataField field,
                 std::optional<Date> date = std::nullopt) {
    try {
        (void)parseAlphaVantageDaily(payload, aapl, today);
        FAIL() << "Expected validation rejection";
    } catch (const MarketDataValidationError& error) {
        EXPECT_EQ(error.issue(), issue);
        EXPECT_EQ(error.field(), field);
        EXPECT_EQ(error.date(), date);
        EXPECT_EQ(std::string(error.what()), issueName(issue));
    }
}

TEST(AlphaVantage, AllEightSymbolsDecodeRawOhlcvAndExcludeToday) {
    EXPECT_EQ(supportedMarketSymbols().size(), 8U);
    for (const auto& symbol : supportedMarketSymbols()) {
        const auto bars =
            parseAlphaVantageDaily(alphaFixture(symbol.value()).dump(), symbol, today);
        ASSERT_EQ(bars.size(), 2U);
        EXPECT_EQ(bars.front().symbol(), symbol);
        EXPECT_EQ(bars.front().date(), from);
        EXPECT_EQ(bars.back().date(), to);
        EXPECT_DOUBLE_EQ(bars[0].open().value(), 100);
        EXPECT_DOUBLE_EQ(bars[0].high().value(), 110);
        EXPECT_DOUBLE_EQ(bars[0].low().value(), 90);
        EXPECT_DOUBLE_EQ(bars[0].close().value(), 105);
        EXPECT_DOUBLE_EQ(bars[0].volume().value(), 1000);
    }
}
TEST(AlphaVantage, RejectsMetadataErrorsDuplicatesAndMalformedNumbers) {
    for (const auto* error : {"Note", "Information", "Error Message"})
        EXPECT_THROW((void)parseAlphaVantageDaily(nlohmann::json{{error, "secret body"}}.dump(),
                                                  aapl, today),
                     MarketDataError);
    EXPECT_THROW((void)parseAlphaVantageDaily("{", aapl, today), MarketDataError);
    EXPECT_THROW((void)parseAlphaVantageDaily("{\"x\":1,\"x\":2}", aapl, today), MarketDataError);
    EXPECT_THROW((void)parseAlphaVantageDaily(alphaFixture("MSFT").dump(), aapl, today),
                 MarketDataError);
    for (const auto* value : {"NaN", "Infinity", "-1", "0", "1e400", "0.100000000000000001"}) {
        auto data = alphaFixture();
        data["Time Series (Daily)"]["2026-01-06"]["1. open"] = value;
        EXPECT_THROW((void)parseAlphaVantageDaily(data.dump(), aapl, today), MarketDataError);
    }
    auto invalid = alphaFixture();
    invalid["Time Series (Daily)"]["2026-01-02"]["5. volume"] = "9007199254740993";
    EXPECT_THROW((void)parseAlphaVantageDaily(invalid.dump(), aapl, today), MarketDataError);
    invalid["Time Series (Daily)"]["2026-01-02"]["5. volume"] = "10000000000000002097152";
    EXPECT_THROW((void)parseAlphaVantageDaily(invalid.dump(), aapl, today), MarketDataError);
    invalid = alphaFixture();
    invalid["Time Series (Daily)"]["2026-01-02"].erase("2. high");
    EXPECT_THROW((void)parseAlphaVantageDaily(invalid.dump(), aapl, today), MarketDataError);
    invalid = alphaFixture();
    invalid["Meta Data"]["5. Time Zone"] = "UTC";
    EXPECT_THROW((void)parseAlphaVantageDaily(invalid.dump(), aapl, today), MarketDataError);
    EXPECT_THROW((void)parseAlphaVantageDaily(alphaFixture().dump(), aapl, to), MarketDataError);
}
TEST(AlphaVantage, BoundedRetriesAreObservableAndRespectDelay) {
    FixtureTransport transport;
    transport.responses = {{503, "", {}}, {429, "", 5}, {200, alphaFixture().dump(), {}}};
    std::vector<std::chrono::seconds> waits;
    std::vector<ProviderAttempt> events;
    AlphaVantageProvider provider(
        transport, "testkey", today, [&](auto e) { events.push_back(e); },
        [&](auto d) { waits.push_back(d); });
    const auto bars = provider.getHistory(aapl, from, to);
    EXPECT_EQ(bars.size(), 2U);
    EXPECT_EQ(transport.calls, 3U);
    EXPECT_EQ(waits, (std::vector<std::chrono::seconds>{std::chrono::seconds{1},
                                                        std::chrono::seconds{5}}));
    EXPECT_EQ(events.size(), 5U);
}
TEST(AlphaVantage, PermanentFailuresDoNotRetryAndTransientRetriesStop) {
    for (long status : {401, 403, 404}) {
        FixtureTransport transport;
        transport.responses = {{status, "secret", {}}};
        AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
        EXPECT_THROW((void)provider.getLatestPrice(aapl), MarketDataError);
        EXPECT_EQ(transport.calls, 1U);
    }
    FixtureTransport transport;
    transport.responses = {{503, "", {}}};
    AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
    EXPECT_THROW((void)provider.getLatestPrice(aapl), TransientMarketDataError);
    EXPECT_EQ(transport.calls, 3U);
    FixtureTransport rate;
    rate.responses = {{429, "", 60}};
    AlphaVantageProvider limited(rate, "testkey", today, {}, [](auto) {});
    EXPECT_THROW((void)limited.getLatestPrice(aapl), MarketDataError);
    EXPECT_EQ(rate.calls, 1U);
}
TEST(AlphaVantage, TransportTimeoutRecoversAndCoverageNeverSilentlyClips) {
    FixtureTransport transport;
    transport.responses = {{200, alphaFixture().dump(), {}}};
    transport.timeout_first = true;
    AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
    EXPECT_DOUBLE_EQ(provider.getLatestPrice(aapl).value(), 115);
    EXPECT_EQ(transport.calls, 2U);
    EXPECT_THROW((void)provider.getHistory(aapl, *Date::create(2026, 1, 1), to), MarketDataError);
    EXPECT_THROW((void)provider.getHistory(aapl, to, from), std::invalid_argument);
    EXPECT_THROW((void)provider.getHistory(aapl, from, today), MarketDataError);
    EXPECT_TRUE(
        provider.getHistory(aapl, *Date::create(2026, 1, 3), *Date::create(2026, 1, 4)).empty());
    EXPECT_THROW((void)provider.getLatestPrice(*Symbol::create("IBM")), MarketDataError);
}
TEST(AlphaVantage, RejectsDuplicateDatesAndBadDatesAndNeverEchoesBodies) {
    const auto fixture = alphaFixture();
    const auto row = fixture["Time Series (Daily)"]["2026-01-02"].dump();
    const auto duplicate = "{\"Meta Data\":" + fixture["Meta Data"].dump() +
                           ",\"Time Series (Daily)\":{\"2026-01-02\":" + row +
                           ",\"2026-01-02\":" + row + "}}";
    EXPECT_THROW((void)parseAlphaVantageDaily(duplicate, aapl, today), MarketDataError);
    EXPECT_THROW((void)parseMarketDate("2026-02-30"), MarketDataError);
    EXPECT_THROW((void)parseMarketDate("2026-1-02"), MarketDataError);
    auto data = fixture;
    data["Time Series (Daily)"]["2026-01-02"]["5. volume"] = "0.5";
    EXPECT_THROW((void)parseAlphaVantageDaily(data.dump(), aapl, today), MarketDataError);
    FixtureTransport transport;
    transport.responses = {{200, "{\"Information\":\"secretapikey\"}", {}}};
    AlphaVantageProvider provider(transport, "testkey", today, {}, [](auto) {});
    try {
        (void)provider.getLatestPrice(aapl);
        FAIL() << "Expected provider error";
    } catch (const MarketDataError& error) {
        EXPECT_EQ(std::string(error.what()).find("secretapikey"), std::string::npos);
    }
    EXPECT_EQ(transport.calls, 1U);
}
TEST(AlphaVantage, SequentialQueriesArePacedWithoutChangingResults) {
    FixtureTransport transport;
    transport.responses = {{200, alphaFixture().dump(), {}}};
    std::vector<std::chrono::seconds> waits;
    AlphaVantageProvider provider(transport, "testkey", today, {},
                                  [&](auto delay) { waits.push_back(delay); });
    EXPECT_DOUBLE_EQ(provider.getLatestPrice(aapl).value(), 115);
    EXPECT_DOUBLE_EQ(provider.getLatestPrice(aapl).value(), 115);
    EXPECT_EQ(waits, (std::vector<std::chrono::seconds>{std::chrono::seconds{2}}));
}

TEST(AlphaVantageValidation, EveryMissingOrInvalidPriceIdentifiesFieldAndDate) {
    const std::vector<std::pair<const char*, MarketDataField>> fields{
        {"1. open", MarketDataField::Open},
        {"2. high", MarketDataField::High},
        {"3. low", MarketDataField::Low},
        {"4. close", MarketDataField::Close}};
    for (const auto& [key, field] : fields) {
        auto data = alphaFixture();
        data["Time Series (Daily)"]["2026-01-02"].erase(key);
        expectIssue(data.dump(), MarketDataIssue::MissingPrice, field, from);
        for (const auto& missing : std::vector<nlohmann::json>{nullptr, "", " \t\r\n"}) {
            data = alphaFixture();
            data["Time Series (Daily)"]["2026-01-02"][key] = missing;
            expectIssue(data.dump(), MarketDataIssue::MissingPrice, field, from);
        }
        for (const auto& invalid : std::vector<nlohmann::json>{
                 "-1", "0", "NaN", "Infinity", "1e400", "0.100000000000000001", 42, true}) {
            data = alphaFixture();
            data["Time Series (Daily)"]["2026-01-02"][key] = invalid;
            expectIssue(data.dump(), MarketDataIssue::InvalidPrice, field, from);
        }
    }
}

TEST(AlphaVantageValidation, EveryImpossibleOhlcRelationRejects) {
    struct BadBound {
        const char* key;
        const char* value;
        MarketDataField field;
    };
    for (const auto& bad :
         std::vector<BadBound>{{"2. high", "80", MarketDataField::High},  // high < low
                               {"1. open", "89", MarketDataField::Open},
                               {"1. open", "111", MarketDataField::Open},
                               {"4. close", "89", MarketDataField::Close},
                               {"4. close", "111", MarketDataField::Close}}) {
        auto data = alphaFixture();
        data["Time Series (Daily)"]["2026-01-02"][bad.key] = bad.value;
        expectIssue(data.dump(), MarketDataIssue::InvalidOhlc, bad.field, from);
    }
}

TEST(AlphaVantageValidation, DuplicateDatesDifferFromDuplicateJsonFields) {
    const auto data = alphaFixture();
    const auto row = data["Time Series (Daily)"]["2026-01-02"].dump();
    for (const auto& second : std::vector<std::string>{row, "{}"}) {
        const auto payload = "{\"Meta Data\":" + data["Meta Data"].dump() +
                             ",\"Time Series (Daily)\":{\"2026-01-02\":" + row +
                             ",\"2026-01-02\":" + second + "}}";
        expectIssue(payload, MarketDataIssue::DuplicateDate, MarketDataField::SessionDate, from);
    }
    expectIssue("{\"secret-key\":1,\"secret-key\":2}", MarketDataIssue::MalformedResponse,
                MarketDataField::None);
}

TEST(AlphaVantageValidation, FutureDatesAndInvalidCalendarDatesAreIdentified) {
    const auto future = *Date::create(2026, 1, 7);
    auto data = alphaFixture();
    data["Meta Data"]["3. Last Refreshed"] = "2026-01-07";
    expectIssue(data.dump(), MarketDataIssue::FutureTimestamp, MarketDataField::LastRefreshed,
                future);
    data = alphaFixture();
    data["Time Series (Daily)"]["2026-01-07"] = data["Time Series (Daily)"]["2026-01-06"];
    expectIssue(data.dump(), MarketDataIssue::FutureTimestamp, MarketDataField::SessionDate,
                future);
    data = alphaFixture();
    data["Meta Data"]["3. Last Refreshed"] = "2026-02-30";
    expectIssue(data.dump(), MarketDataIssue::InvalidDate, MarketDataField::LastRefreshed);
    data = alphaFixture();
    data["Time Series (Daily)"]["2026-02-30"] = data["Time Series (Daily)"]["2026-01-06"];
    expectIssue(data.dump(), MarketDataIssue::InvalidDate, MarketDataField::SessionDate);
}

TEST(AlphaVantageValidation, RejectedTodayOrOutOfRangeBarsNeverRetryOrReachCaller) {
    for (const auto* date : {"2026-01-02", "2026-01-06"}) {
        auto data = alphaFixture();
        data["Time Series (Daily)"][date]["2. high"] = "1";
        FixtureTransport transport;
        transport.responses = {{200, data.dump(), {}}};
        unsigned waits = 0;
        AlphaVantageProvider provider(transport, "testkey", today, {}, [&](auto) { ++waits; });
        try {
            (void)provider.getHistory(aapl, to, to);
            FAIL() << "Expected full response rejection";
        } catch (const MarketDataValidationError& error) {
            EXPECT_EQ(error.issue(), MarketDataIssue::InvalidOhlc);
        }
        EXPECT_EQ(transport.calls, 1U);
        EXPECT_EQ(waits, 0U);
    }
}

TEST(AlphaVantageValidation, InvalidVolumeHasSeparateSafeContext) {
    for (const auto& value :
         std::vector<nlohmann::json>{nullptr, "-1", "0.5", "NaN", "10000000000000002097152"}) {
        auto data = alphaFixture();
        data["Time Series (Daily)"]["2026-01-02"]["5. volume"] = value;
        expectIssue(data.dump(), MarketDataIssue::InvalidVolume, MarketDataField::Volume, from);
    }
}
}  // namespace
