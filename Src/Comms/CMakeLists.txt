# Communications module

# Add Serial subdirectory
add_subdirectory(Serial)

# Create object library for Comms
add_library(NfcCpp_Comms OBJECT)

# Add source files from Serial subdirectory
target_sources(NfcCpp_Comms
    PRIVATE
        $<TARGET_OBJECTS:NfcCpp_Comms_Serial>
)

# Include directories
target_include_directories(NfcCpp_Comms
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

# Link dependencies
target_link_libraries(NfcCpp_Comms
    PRIVATE
        etl::etl
)
