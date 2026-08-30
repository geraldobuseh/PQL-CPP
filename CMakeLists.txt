cmake_minimum_required(VERSION 3.20)

project(
    PersonalQuantLab
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Personal Quant Lab - C++20 Financial Analysis System"
)

# C++ Standard Configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Flags for Clean Build
if(MSVC)
    # MSVC: Enable /W4 warnings, treat warnings as errors
    add_compile_options(/W4 /WX)
    # Suppress some overly-aggressive warnings for external dependencies
    add_compile_options(/wd4068)  # unknown pragma
    add_compile_options(/wd4996)  # deprecated function (for std lib)
else()
    # GCC/Clang: Enable pedantic warnings
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Enable testing
enable_testing()

# Add source subdirectories
add_subdirectory(src)
add_subdirectory(test)

# Summary
message(STATUS "===== Personal Quant Lab Build Configuration =====")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "===================================================")
