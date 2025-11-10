# RC522 NFC reader driver module (placeholder)

# Create object library
add_library(NfcCpp_Rc522 OBJECT)

# Add source files
target_sources(NfcCpp_Rc522
    PRIVATE
        Rc522Driver.cpp
        Rc522ApduAdapter.cpp
)

# Include directories
target_include_directories(NfcCpp_Rc522
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

# Link dependencies
target_link_libraries(NfcCpp_Rc522
    PRIVATE
        etl::etl
)
