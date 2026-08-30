// Personal Quant Lab - Smoke Test
// Minimal test to verify build system and GoogleTest integration

#include <gtest/gtest.h>

// Test: Basic arithmetic (sanity check)
TEST(SmokeTest, BasicArithmetic) {
    int a = 2;
    int b = 3;
    int result = a + b;
    EXPECT_EQ(result, 5);
}

// Test: String handling (C++20 compatible)
TEST(SmokeTest, StringHandling) {
    std::string message = "Personal Quant Lab";
    EXPECT_EQ(message.length(), 18);
    EXPECT_NE(message, "");
}

// Test: Simple assertion
TEST(SmokeTest, SimpleAssertion) {
    bool condition = true;
    ASSERT_TRUE(condition);
}
