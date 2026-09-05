# Personal Quant Lab - Test Configuration

# Fetch GoogleTest from GitHub
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

# For Windows: prevent overriding parent project compiler options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Create test executable
add_executable(pql_tests test_smoke.cpp)

# Link against GoogleTest
target_link_libraries(pql_tests PUBLIC gtest_main)

# Set C++ standard
target_compile_features(pql_tests PRIVATE cxx_std_20)

# Suppress GoogleTest's overly-aggressive warnings (external dependency)
if(MSVC)
    target_compile_options(gtest PUBLIC /wd4251)  # class needs dll-interface
    target_compile_options(gtest_main PUBLIC /wd4251)
endif()

# Register test with CTest
add_test(NAME SmokeTest COMMAND pql_tests)

add_executable(pql_domain_tests test_financial_types.cpp)
target_link_libraries(pql_domain_tests PRIVATE pql_domain gtest_main)
add_test(NAME FinancialTypesTest COMMAND pql_domain_tests)

add_executable(pql_order_trade_tests test_order_trade.cpp)
target_link_libraries(pql_order_trade_tests PRIVATE pql_domain gtest_main)
add_test(NAME OrderTradeTest COMMAND pql_order_trade_tests)

add_executable(pql_position_tests test_position.cpp)
target_link_libraries(pql_position_tests PRIVATE pql_domain gtest_main)
add_test(NAME PositionTest COMMAND pql_position_tests)

add_executable(pql_portfolio_tests test_portfolio.cpp)
target_link_libraries(pql_portfolio_tests PRIVATE pql_domain gtest_main)
add_test(NAME PortfolioTest COMMAND pql_portfolio_tests)
