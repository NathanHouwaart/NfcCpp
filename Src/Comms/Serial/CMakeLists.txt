# Serial communication module

# Create object library
add_library(NfcCpp_Comms_Serial OBJECT)

# Add source files
target_sources(NfcCpp_Comms_Serial
    PRIVATE
        SerialBusWin.cpp
)

# Include directories
target_include_directories(NfcCpp_Comms_Serial
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

# Link dependencies
target_link_libraries(NfcCpp_Comms_Serial
    PRIVATE
        etl::etl
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(NfcCpp_Comms_Serial PRIVATE _WIN32_WINNT=0x0601)
endif()
