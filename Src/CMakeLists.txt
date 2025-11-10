# NfcCpp Library Source

# Add subdirectories
add_subdirectory(Comms)
add_subdirectory(Pn532)
add_subdirectory(Rc522)

# Create library target
if(NFCCPP_BUILD_SHARED_LIBS)
    add_library(NfcCpp SHARED)
else()
    add_library(NfcCpp STATIC)
endif()

# Add alias for consistent usage
add_library(NfcCpp::NfcCpp ALIAS NfcCpp)

# Collect all source files from subdirectories
target_sources(NfcCpp
    PRIVATE
        $<TARGET_OBJECTS:NfcCpp_Comms_Serial>
        $<TARGET_OBJECTS:NfcCpp_Pn532>
        $<TARGET_OBJECTS:NfcCpp_Rc522>
)

# Include directories
target_include_directories(NfcCpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Include>
        $<INSTALL_INTERFACE:include/NfcCpp>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
)

# Link external dependencies
# Note: External dependencies are linked privately and their symbols are embedded
# Users of NfcCpp don't need to link against them directly
target_link_libraries(NfcCpp
    PRIVATE
        etl::etl
        tiny-aes
        libcppdes
)

# Set library properties
set_target_properties(NfcCpp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "nfccpp"
    EXPORT_NAME NfcCpp
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(NfcCpp PRIVATE _WIN32_WINNT=0x0601)
endif()
