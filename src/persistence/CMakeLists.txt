add_library(pql_persistence STATIC postgres.cpp)
target_include_directories(pql_persistence PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(pql_persistence PUBLIC pql_domain PRIVATE libpqxx::pqxx)
target_compile_features(pql_persistence PUBLIC cxx_std_20)
