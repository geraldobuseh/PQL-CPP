#pragma once
#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include "market_data/market_data_provider.hpp"

namespace pql {
struct HttpResponse {
    long status;
    std::string body;
    std::optional<unsigned> retry_after_seconds;
};
class TransientMarketDataError : public MarketDataError {
 public: using MarketDataError::MarketDataError;
};
class HttpTransport {
 public:
    virtual ~HttpTransport() = default;
    virtual HttpResponse get(const std::string& url) = 0;
};
// TLS verified, no redirects, bounded response size and connection/request times.
class CurlHttpTransport final : public HttpTransport {
 public: HttpResponse get(const std::string& url) override;
};
struct ProviderAttempt { Symbol symbol; unsigned attempt; bool retrying; };
using RetryObserver = std::function<void(const ProviderAttempt&)>;
using RetryWait = std::function<void(std::chrono::seconds)>;

[[nodiscard]] const std::vector<Symbol>& supportedMarketSymbols();
[[nodiscard]] Date parseMarketDate(const std::string& text);
[[nodiscard]] std::string formatMarketDate(Date date);
// Full response validation before filtering. Duplicate JSON keys always reject.
[[nodiscard]] std::vector<PriceBar> parseAlphaVantageDaily(
    const std::string& response, const Symbol& symbol, Date today);

class AlphaVantageProvider final : public MarketDataProvider {
 public:
    // Borrowed transport must outlive provider. Capture today once at composition;
    // construct a new provider for a new ingestion run. Never log the API key/URL.
    AlphaVantageProvider(HttpTransport& transport, std::string api_key, Date today,
                         RetryObserver observer = {}, RetryWait wait = {});
    Price getLatestPrice(const Symbol& symbol) override;
    std::vector<PriceBar> getHistory(const Symbol& symbol, Date from, Date to) override;
 private:
    std::vector<PriceBar> fetch(const Symbol& symbol);
    HttpTransport& transport_;
    std::string api_key_;
    Date today_;
    RetryObserver observer_;
    RetryWait wait_;
    bool requested_{false}; // Sequential use only; pace successive symbol requests.
};
} // namespace pql
