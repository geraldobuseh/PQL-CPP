# Personal Quant Lab - Library and Executable Configuration

# Create the main executable
add_executable(pql_app main.cpp)

# Set target properties
target_compile_features(pql_app PRIVATE cxx_std_20)

# Install target (optional)
install(TARGETS pql_app DESTINATION bin)
