# PN532 NFC driver module

# Create object library
add_library(NfcCpp_Pn532 OBJECT)

# Add source files
target_sources(NfcCpp_Pn532
    PRIVATE
        Pn532Driver.cpp
        Pn532ApduAdapter.cpp
        Pn532RequestFrame.cpp
        Pn532ResponseFrame.cpp
        CommandRequest.cpp
        CommandResponse.cpp
        Commands/GetFirmwareVersion.cpp
        Commands/GetGeneralStatus.cpp
        Commands/PerformSelfTest.cpp
        Commands/SAMConfiguration.cpp
        Commands/RFConfiguration.cpp
        Commands/InListPassiveTarget.cpp
        Commands/InDataExchange.cpp
)

# Include directories
target_include_directories(NfcCpp_Pn532
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Src
        ${CMAKE_SOURCE_DIR}/external/etl/include
)

# Link dependencies
target_link_libraries(NfcCpp_Pn532
    PRIVATE
        etl::etl
)
