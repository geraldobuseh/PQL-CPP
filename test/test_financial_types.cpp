#include <gtest/gtest.h>

#include <limits>
#include <type_traits>
#include <utility>

#include "domain/financial_types.hpp"

namespace {

using namespace pql;

// Check every ordered pair: domains cannot be constructed, assigned or implicitly
// converted from one another. The same-type case checks ordinary value semantics.
template <typename Left, typename Right>
constexpr bool distinct_or_same() {
    if constexpr (std::is_same_v<Left, Right>) {
        return std::is_copy_constructible_v<Left> && std::is_copy_assignable_v<Left>;
    } else {
        return !std::is_constructible_v<Left, Right> && !std::is_convertible_v<Right, Left> &&
               !std::is_assignable_v<Left&, Right>;
    }
}

template <typename Type>
constexpr bool isolated_domain() {
    return !std::is_default_constructible_v<Type> && !std::is_aggregate_v<Type> &&
           distinct_or_same<Type, Symbol>() && distinct_or_same<Type, Money>() &&
           distinct_or_same<Type, Quantity>() && distinct_or_same<Type, Price>() &&
           distinct_or_same<Type, Timestamp>() && distinct_or_same<Type, OrderId>() &&
           distinct_or_same<Type, PortfolioId>() && distinct_or_same<Type, StrategyId>();
}

static_assert(isolated_domain<Symbol>());
static_assert(isolated_domain<Money>());
static_assert(isolated_domain<Quantity>());
static_assert(isolated_domain<Price>());
static_assert(isolated_domain<Timestamp>());
static_assert(isolated_domain<OrderId>());
static_assert(isolated_domain<PortfolioId>());
static_assert(isolated_domain<StrategyId>());

template <typename Domain, typename Primitive>
constexpr bool explicit_boundary() {
    using Access = decltype(std::declval<Domain&>().value());
    return !std::is_convertible_v<Primitive, Domain> && !std::is_convertible_v<Domain, Primitive> &&
           (!std::is_reference_v<Access> || !std::is_assignable_v<Access, Primitive>);
}

static_assert(explicit_boundary<Symbol, std::string>());
static_assert(explicit_boundary<Money, double>());
static_assert(explicit_boundary<Quantity, double>());
static_assert(explicit_boundary<Price, double>());
static_assert(explicit_boundary<Timestamp, Timestamp::Value>());
static_assert(explicit_boundary<OrderId, std::int64_t>());
static_assert(explicit_boundary<PortfolioId, std::int64_t>());
static_assert(explicit_boundary<StrategyId, std::int64_t>());

TEST(SymbolTest, PreservesValidSymbolsAndOwnsInput) {
    for (const auto input : {"SPY", "BRK.B", "BRK-B", "spy", "7203"}) {
        const auto symbol = Symbol::create(input);
        ASSERT_TRUE(symbol);
        EXPECT_EQ(symbol->value(), input);
        EXPECT_EQ(symbol, Symbol::create(input));
    }
    EXPECT_NE(Symbol::create("SPY"), Symbol::create("spy"));
    std::string input = "SPY";
    const auto symbol = Symbol::create(input);
    input[0] = 'X';
    ASSERT_TRUE(symbol);
    EXPECT_EQ(symbol->value(), "SPY");
}

TEST(SymbolTest, RejectsMalformedInputWithoutNormalizing) {
    for (const auto input : {"", " ", " SPY", "SPY ", "S PY", "SPY\n", "SP\tY", ".SPY", "-SPY",
                             "SP/Y", "SP_Y", "SP\xFF"}) {
        EXPECT_FALSE(Symbol::create(input)) << input;
    }
    EXPECT_FALSE(Symbol::create(std::string_view{"SP\0Y", 4}));
}

TEST(SymbolTest, CopyAndMovePreserveBothValues) {
    auto source = Symbol::create("SPY");
    ASSERT_TRUE(source);
    const auto copied = *source;
    const auto moved = std::move(*source);
    EXPECT_EQ(copied, moved);
    EXPECT_EQ(source->value(), "SPY");
    auto destination = Symbol::create("AAPL");
    ASSERT_TRUE(destination);
    *destination = std::move(*source);
    EXPECT_EQ(destination->value(), "SPY");
    EXPECT_EQ(source->value(), "SPY");
}

TEST(MoneyTest, AcceptsSignedFiniteAmounts) {
    for (const double input : {-std::numeric_limits<double>::max(), -12.5, -0.0, 0.0, 12.5,
                               std::numeric_limits<double>::max()}) {
        const auto money = Money::create(input);
        ASSERT_TRUE(money);
        EXPECT_DOUBLE_EQ(money->value(), input);
        EXPECT_EQ(money, Money::create(input));
    }
    EXPECT_NE(Money::create(1.0), Money::create(2.0));
}

TEST(QuantityTest, AcceptsZeroAndFractionalMagnitudes) {
    for (const double input : {-0.0, 0.0, 0.125, std::numeric_limits<double>::max()}) {
        const auto quantity = Quantity::create(input);
        ASSERT_TRUE(quantity);
        EXPECT_DOUBLE_EQ(quantity->value(), input);
        EXPECT_EQ(quantity, Quantity::create(input));
    }
    EXPECT_FALSE(Quantity::create(-0.125));
    EXPECT_FALSE(Quantity::create(-std::numeric_limits<double>::max()));
    EXPECT_NE(Quantity::create(1.0), Quantity::create(2.0));
}

TEST(PriceTest, RequiresPositiveEquityQuotes) {
    for (const double input :
         {std::numeric_limits<double>::min(), 0.125, 500.0, std::numeric_limits<double>::max()}) {
        const auto price = Price::create(input);
        ASSERT_TRUE(price);
        EXPECT_DOUBLE_EQ(price->value(), input);
        EXPECT_EQ(price, Price::create(input));
    }
    EXPECT_FALSE(Price::create(0.0));
    EXPECT_FALSE(Price::create(-0.0));
    EXPECT_FALSE(Price::create(-0.125));
    EXPECT_NE(Price::create(1.0), Price::create(2.0));
}

TEST(FinancialNumbersTest, RejectsNonFiniteValues) {
    for (const double input :
         {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
          -std::numeric_limits<double>::infinity()}) {
        EXPECT_FALSE(Money::create(input));
        EXPECT_FALSE(Quantity::create(input));
        EXPECT_FALSE(Price::create(input));
    }
}

template <typename Id>
class IdentifierTest : public ::testing::Test {};
using IdentifierTypes = ::testing::Types<OrderId, PortfolioId, StrategyId>;
TYPED_TEST_SUITE(IdentifierTest, IdentifierTypes);

TYPED_TEST(IdentifierTest, AcceptsPositiveSigned64BitValues) {
    for (const std::int64_t input : {std::int64_t{1}, std::numeric_limits<std::int64_t>::max()}) {
        const auto id = TypeParam::create(input);
        ASSERT_TRUE(id);
        EXPECT_EQ(id->value(), input);
        EXPECT_EQ(id, TypeParam::create(input));
    }
    EXPECT_NE(TypeParam::create(1), TypeParam::create(2));
}

TYPED_TEST(IdentifierTest, RejectsZeroAndNegativeValues) {
    EXPECT_FALSE(TypeParam::create(0));
    EXPECT_FALSE(TypeParam::create(-1));
    EXPECT_FALSE(TypeParam::create(std::numeric_limits<std::int64_t>::min()));
}

TEST(TimestampTest, PreservesMillisecondInstantsIncludingBeforeEpoch) {
    for (const auto ticks : {-1LL, 0LL, 1LL, 1234567890123LL}) {
        const Timestamp::Value instant{std::chrono::milliseconds{ticks}};
        const Timestamp timestamp{instant};
        EXPECT_EQ(timestamp.value(), instant);
        EXPECT_EQ(timestamp, Timestamp{instant});
    }
    const Timestamp before{Timestamp::Value{std::chrono::milliseconds{-1}}};
    const Timestamp epoch{Timestamp::Value{}};
    const Timestamp after{Timestamp::Value{std::chrono::milliseconds{1}}};
    EXPECT_LT(before, epoch);
    EXPECT_LT(epoch, after);
    EXPECT_NE(before, after);
}

}  // namespace
