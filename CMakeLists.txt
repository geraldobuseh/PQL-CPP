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

# Sanitizers Configuration
option(PQL_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)

if(PQL_ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC: AddressSanitizer support (limited)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        # GCC/Clang: Full sanitizer suite
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

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
option(PQL_BUILD_PERSISTENCE "Build PostgreSQL repositories (requires libpqxx 7.10+)" OFF)
option(PQL_BUILD_POSTGRES_TESTS "Build integration tests requiring an isolated migrated PostgreSQL database" OFF)
if(PQL_BUILD_POSTGRES_TESTS AND NOT PQL_BUILD_PERSISTENCE)
    message(FATAL_ERROR "PQL_BUILD_POSTGRES_TESTS requires PQL_BUILD_PERSISTENCE")
endif()
if(PQL_BUILD_PERSISTENCE)
    find_package(libpqxx 7.10 CONFIG QUIET)
    if(NOT TARGET libpqxx::pqxx)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx>=7.10)
        add_library(libpqxx::pqxx ALIAS PkgConfig::PQXX)
    endif()
endif()
add_subdirectory(src)
add_subdirectory(test)

# Summary
message(STATUS "===== Personal Quant Lab Build Configuration =====")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Generator: ${CMAKE_GENERATOR}")
message(STATUS "===================================================")
