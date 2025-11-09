# Tests for NfcCpp Library

# Enable testing
enable_testing()

# Google Test is already fetched by cppDes, so we can use it
# If not available, we would add:
# include(FetchContent)
# FetchContent_Declare(...)

# Add test subdirectories
add_subdirectory(unit)
add_subdirectory(integration)
