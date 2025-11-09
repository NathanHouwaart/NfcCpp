cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

project(NfcCpp VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(NFCCPP_BUILD_EXAMPLES "Build example applications" ON)
option(NFCCPP_BUILD_TESTS "Build unit tests" ON)
option(NFCCPP_BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Add external dependencies
add_subdirectory(external/etl)
add_subdirectory(external/tiny-aes)
add_subdirectory(external/cppDes)

# Add source directory (builds the library)
add_subdirectory(Src)

# Add examples if enabled
if(NFCCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add tests if enabled
if(NFCCPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Optional: Installation rules (disabled by default due to external dependency complexity)
# To enable installation, set NFCCPP_ENABLE_INSTALL=ON
option(NFCCPP_ENABLE_INSTALL "Enable installation" OFF)

if(NFCCPP_ENABLE_INSTALL)
    include(GNUInstallDirs)
    
    # Install headers
    install(DIRECTORY Include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/NfcCpp
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
    
    # Install library
    install(TARGETS NfcCpp
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
