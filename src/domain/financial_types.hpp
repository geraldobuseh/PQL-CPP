#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pql {

// Case-sensitive ASCII symbol: [A-Za-z0-9][A-Za-z0-9.-]*. No normalization.
class Symbol {
   public:
    [[nodiscard]] static std::optional<Symbol> create(std::string_view value);

    // Copy even from rvalues so a moved-from Symbol cannot become empty.
    Symbol(const Symbol&) = default;
    Symbol& operator=(const Symbol&) = default;

    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    bool operator==(const Symbol&) const = default;

   private:
    explicit Symbol(std::string_view value) : value_(value) {}
    std::string value_;
};

// Approximate, single-currency amount. Negative amounts represent debits/losses.
class Money {
   public:
    [[nodiscard]] static std::optional<Money> create(double value);
    [[nodiscard]] double value() const noexcept { return value_; }
    bool operator==(const Money&) const = default;

   private:
    explicit Money(double value) : value_(value) {}
    double value_;
};

// Nonnegative magnitude, including fractional units and zero; not a signed position.
class Quantity {
   public:
    [[nodiscard]] static std::optional<Quantity> create(double value);
    [[nodiscard]] double value() const noexcept { return value_; }
    bool operator==(const Quantity&) const = default;

   private:
    explicit Quantity(double value) : value_(value) {}
    double value_;
};

// Strictly positive equity quote; not a generic valuation or derivative price.
class Price {
   public:
    [[nodiscard]] static std::optional<Price> create(double value);
    [[nodiscard]] double value() const noexcept { return value_; }
    bool operator==(const Price&) const = default;

   private:
    explicit Price(double value) : value_(value) {}
    double value_;
};

// System-clock instant, milliseconds since the Unix epoch; no local time zone.
// Epoch, pre-epoch and future instants are valid. No implicit clock reads.
class Timestamp {
   public:
    using Value = std::chrono::sys_time<std::chrono::milliseconds>;

    explicit Timestamp(Value value) noexcept : value_(value) {}
    [[nodiscard]] Value value() const noexcept { return value_; }
    auto operator<=>(const Timestamp&) const = default;

   private:
    Value value_;
};

// Positive signed 64-bit identifiers. Allocation/uniqueness belongs to persistence.
class OrderId {
   public:
    [[nodiscard]] static std::optional<OrderId> create(std::int64_t value);
    [[nodiscard]] std::int64_t value() const noexcept { return value_; }
    bool operator==(const OrderId&) const = default;

   private:
    explicit OrderId(std::int64_t value) : value_(value) {}
    std::int64_t value_;
};

class PortfolioId {
   public:
    [[nodiscard]] static std::optional<PortfolioId> create(std::int64_t value);
    [[nodiscard]] std::int64_t value() const noexcept { return value_; }
    bool operator==(const PortfolioId&) const = default;

   private:
    explicit PortfolioId(std::int64_t value) : value_(value) {}
    std::int64_t value_;
};

class StrategyId {
   public:
    [[nodiscard]] static std::optional<StrategyId> create(std::int64_t value);
    [[nodiscard]] std::int64_t value() const noexcept { return value_; }
    bool operator==(const StrategyId&) const = default;

   private:
    explicit StrategyId(std::int64_t value) : value_(value) {}
    std::int64_t value_;
};

}  // namespace pql
