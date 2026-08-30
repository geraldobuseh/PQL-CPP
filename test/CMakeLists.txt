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
