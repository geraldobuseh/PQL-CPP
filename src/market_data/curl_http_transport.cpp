#include <curl/curl.h>

#include <algorithm>
#include <charconv>
#include <limits>
#include <memory>

#include "market_data/alpha_vantage.hpp"

namespace pql {
namespace {
struct CurlRuntime {
    CurlRuntime() {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
            throw MarketDataError("HTTP initialization failed");
    }
    ~CurlRuntime() { curl_global_cleanup(); }
};
struct Buffer {
    std::string body;
    std::optional<unsigned> retry;
    bool failed{false};
};
std::size_t receive(char* data, std::size_t size, std::size_t count, void* context) noexcept {
    auto& buffer = *static_cast<Buffer*>(context);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) return 0;
    const auto bytes = size * count;
    if (bytes > 2 * 1024 * 1024 - buffer.body.size()) {
        buffer.failed = true;
        return 0;
    }
    try {
        buffer.body.append(data, bytes);
        return bytes;
    } catch (...) {
        buffer.failed = true;
        return 0;
    }
}
std::size_t header(char* data, std::size_t size, std::size_t count, void* context) noexcept {
    auto& buffer = *static_cast<Buffer*>(context);
    if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) return 0;
    const auto bytes = size * count;
    try {
        std::string line(data, bytes);
        const auto colon = line.find(':');
        if (colon != std::string::npos) {
            auto name = line.substr(0, colon);
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                return char(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
            });
            if (name == "retry-after") {
                const auto start = line.find_first_not_of(" \t", colon + 1);
                if (start == std::string::npos)
                    buffer.retry = 31;
                else {
                    const auto end = line.find_last_not_of(" \t\r\n");
                    const auto value = line.substr(start, end - start + 1);
                    unsigned seconds{};
                    const auto parsed =
                        std::from_chars(value.data(), value.data() + value.size(), seconds);
                    // Unrecognized/date-form delays defer to a later run rather
                    // than retry sooner than the provider requested.
                    buffer.retry =
                        parsed.ec == std::errc{} && parsed.ptr == value.data() + value.size()
                            ? seconds
                            : 31;
                }
            }
        }
        return bytes;
    } catch (...) {
        buffer.failed = true;
        return 0;
    }
}
}  // namespace
HttpResponse CurlHttpTransport::get(const std::string& url) {
    if (!url.starts_with("https://www.alphavantage.co/query?") ||
        url.find('\0') != std::string::npos)
        throw MarketDataError("Unsupported HTTP endpoint");
    static const CurlRuntime runtime;
    (void)runtime;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
    if (!handle) throw MarketDataError("HTTP allocation failed");
    Buffer buffer;
    auto check = [](CURLcode code) {
        if (code != CURLE_OK) throw MarketDataError("HTTP configuration failed");
    };
    check(curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str()));
    check(curl_easy_setopt(handle.get(), CURLOPT_PROTOCOLS_STR, "https"));
    check(curl_easy_setopt(handle.get(), CURLOPT_FOLLOWLOCATION, 0L));
    check(curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYPEER, 1L));
    check(curl_easy_setopt(handle.get(), CURLOPT_SSL_VERIFYHOST, 2L));
    check(curl_easy_setopt(handle.get(), CURLOPT_CONNECTTIMEOUT, 10L));
    check(curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT, 30L));
    check(curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L));
    check(curl_easy_setopt(handle.get(), CURLOPT_USERAGENT, "PersonalQuantLab/0.1"));
    check(curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, receive));
    check(curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &buffer));
    check(curl_easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, header));
    check(curl_easy_setopt(handle.get(), CURLOPT_HEADERDATA, &buffer));
    const auto code = curl_easy_perform(handle.get());
    if (buffer.failed)
        throw MarketDataError("Provider response exceeded limits or could not be buffered");
    if (code == CURLE_OPERATION_TIMEDOUT || code == CURLE_COULDNT_CONNECT ||
        code == CURLE_COULDNT_RESOLVE_HOST || code == CURLE_RECV_ERROR ||
        code == CURLE_SEND_ERROR || code == CURLE_PARTIAL_FILE)
        throw TransientMarketDataError("Transient provider transport failure");
    if (code != CURLE_OK) throw MarketDataError("Provider TLS or transport request failed");
    long status{};
    check(curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status));
    return {status, std::move(buffer.body), buffer.retry};
}
}  // namespace pql
