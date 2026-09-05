# Personal Quant Lab - Library and Executable Configuration

add_library(pql_domain STATIC
    domain/financial_types.cpp
    domain/order.cpp
    domain/trade.cpp
    domain/position.cpp
    domain/portfolio.cpp
    domain/transaction.cpp
)
target_include_directories(pql_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(pql_domain PUBLIC cxx_std_20)
if(PQL_BUILD_PERSISTENCE)
    add_subdirectory(persistence)
endif()

# Create the main executable
add_executable(pql_app main.cpp)
target_link_libraries(pql_app PRIVATE pql_domain)

# Set target properties
target_compile_features(pql_app PRIVATE cxx_std_20)

# Install target (optional)
install(TARGETS pql_app DESTINATION bin)
