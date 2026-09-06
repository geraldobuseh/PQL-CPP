#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>

#include "market_data/alpha_vantage.hpp"
#include "market_data/validation.hpp"
#include "persistence/postgres.hpp"

namespace {
void log(nlohmann::json event) {
    event["source"] = "alphavantage.daily";
    std::cout << event.dump() << std::endl;
}
}  // namespace
int main(int argc, char** argv) {
    using namespace pql;
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: pql_ingest_daily FROM TO [SYMBOL|all] [--fetch-only]\n";
        return 2;
    }
    try {
        const auto from = parseMarketDate(argv[1]);
        const auto to = parseMarketDate(argv[2]);
        if (from > to) throw MarketDataError("Reversed ingestion range");
        const bool fetch_only = argc == 5 && std::string(argv[4]) == "--fetch-only";
        if (argc == 5 && !fetch_only) throw MarketDataError("Unknown ingestion option");
        auto symbols = supportedMarketSymbols();
        if (argc >= 4 && std::string(argv[3]) != "all") {
            const auto symbol = Symbol::create(argv[3]);
            if (!symbol || std::find(symbols.begin(), symbols.end(), *symbol) == symbols.end())
                throw MarketDataError("Unsupported ingestion symbol");
            symbols = {*symbol};
        }
        const auto calendar = std::chrono::year_month_day{
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
        const auto today = Date::create(int(calendar.year()), unsigned(calendar.month()),
                                        unsigned(calendar.day()));
        if (!today) throw MarketDataError("Unsupported current date");
        const auto* key = std::getenv("ALPHAVANTAGE_API_KEY");
        if (!key || !*key)
            throw MarketDataError("Configure ALPHAVANTAGE_API_KEY in the environment");
        CurlHttpTransport transport;
        AlphaVantageProvider provider(transport, key, *today, [](const ProviderAttempt& event) {
            log({{"event", event.retrying ? "provider_retry" : "provider_request"},
                 {"symbol", event.symbol.value()},
                 {"attempt", event.attempt}});
        });
        unsigned failures = 0;
        log({{"event", "ingestion_start"},
             {"from", formatMarketDate(from)},
             {"to", formatMarketDate(to)},
             {"symbols", symbols.size()},
             {"fetch_only", fetch_only}});
        for (const auto& symbol : symbols) {
            const auto start = std::chrono::steady_clock::now();
            try {
                const auto bars = provider.getHistory(symbol, from, to);
                if (fetch_only) {
                    log({{"event", "fetch_complete"},
                         {"symbol", symbol.value()},
                         {"fetched", bars.size()}});
                    continue;
                }
                persistence::PostgresUnitOfWork work;
                const auto counts = work.storeDailyBars(symbol, "alphavantage.daily", bars);
                work.commit();
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - start)
                                         .count();
                log({{"event", "symbol_committed"},
                     {"symbol", symbol.value()},
                     {"fetched", bars.size()},
                     {"inserted", counts.inserted},
                     {"unchanged", counts.unchanged},
                     {"elapsed_ms", elapsed}});
            } catch (const persistence::CommitUncertain&) {
                ++failures;
                log({{"event", "symbol_commit_uncertain"}, {"symbol", symbol.value()}});
            } catch (const MarketDataValidationError& error) {
                ++failures;
                log({{"event", "symbol_failed"},
                     {"symbol", symbol.value()},
                     {"quality_code", issueName(error.issue())},
                     {"field", fieldName(error.field())},
                     {"session_date", error.date() ? nlohmann::json(formatMarketDate(*error.date()))
                                                   : nlohmann::json(nullptr)}});
            } catch (const MarketDataError& error) {
                ++failures;
                log({{"event", "symbol_failed"},
                     {"symbol", symbol.value()},
                     {"reason", error.what()}});
            } catch (const persistence::PersistenceError& error) {
                ++failures;
                log({{"event", "symbol_failed"},
                     {"symbol", symbol.value()},
                     {"reason", error.what()}});
            }
        }
        log({{"event", "ingestion_finished"}, {"failed_symbols", failures}});
        return failures == 0 ? 0 : 1;
    } catch (const MarketDataError& error) {
        log({{"event", "ingestion_failed"}, {"reason", error.what()}});
    } catch (const std::exception&) {
        log({{"event", "ingestion_failed"}, {"reason", "Unexpected local failure"}});
    }
    return 1;
}
