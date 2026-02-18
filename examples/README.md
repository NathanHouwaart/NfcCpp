# NfcCpp Examples

This directory contains example applications demonstrating how to use the NfcCpp library.

## Building Examples

The examples are built automatically when you build the main project with the `NFCCPP_BUILD_EXAMPLES` option enabled (default):

```bash
cmake -B build -S .
cmake --build build
```

To disable building examples:

```bash
cmake -B build -S . -DNFCCPP_BUILD_EXAMPLES=OFF
cmake --build build
```

## Running Examples

After building, the example executables will be in the `build/examples` directory:

```bash
# On Windows
.\build\examples\basic_pn532_example.exe
.\build\examples\serial_communication_example.exe
.\build\examples\card_detection_example.exe
```

## Available Examples

### 1. Basic PN532 (`basic_pn532`)
Demonstrates basic initialization and usage of the PN532 NFC reader driver.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

### 2. Serial Communication (`serial_communication`)
Shows how to use the serial communication interface for Windows.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

### 3. Card Detection (`card_detection`)
Illustrates how to detect and identify NFC cards.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration

### 4. DESFire Authenticate + ChangeKey (`desfire_auth_changekey`)
Shows a full DESFire flow: `SelectApplication -> Authenticate -> ChangeKey`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 5. DESFire Authenticate (`desfire_authenticate`)
Shows a minimal DESFire auth flow: `SelectApplication -> Authenticate`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 6. DESFire CreateApplication (`desfire_create_application`)
Shows a full DESFire flow: `Select PICC -> Authenticate PICC -> CreateApplication -> Select app -> Authenticate app`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 7. DESFire DeleteApplication (`desfire_delete_application`)
Shows a PICC-level DESFire flow: `Select PICC -> Authenticate PICC -> DeleteApplication`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 8. DESFire GetVersion (`desfire_get_version`)
Reads DESFire version/manufacturing payload and prints raw + decoded fields.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 9. DESFire GetApplicationIDs (`desfire_get_application_ids`)
Lists available application IDs on the PICC (optionally after PICC authentication).

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 10. DESFire GetCardUID (`desfire_get_card_uid`)
Reads real card UID using PICC authentication and `GetCardUID`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 11. DESFire GetKeySettings + GetKeyVersion (`desfire_get_key_info`)
Reads key settings and key version for a selected AID.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 12. DESFire CreateStdDataFile (`desfire_create_std_data_file`)
Creates a standard data file in a selected DESFire application after authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 13. DESFire GetFileIDs (`desfire_get_file_ids`)
Lists file IDs for the selected DESFire application.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 14. DESFire GetFileSettings (`desfire_get_file_settings`)
Reads and decodes file settings for one file in the selected application.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 15. DESFire ReadData + WriteData (`desfire_read_write_data`)
Writes bytes to a standard data file and/or reads bytes back from a selected application file.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 16. DESFire ChangeKeySettings (`desfire_change_key_settings`)
Changes `KeySettings1` on the selected PICC/application, with helper options for individual bit fields.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 17. DESFire SetConfiguration (`desfire_set_configuration`)
Changes PICC configuration (`SetConfiguration`), including PICC config flags and ATS payload.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 18. DESFire CreateBackupDataFile (`desfire_create_backup_data_file`)
Creates a backup data file in a selected DESFire application after authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 19. DESFire CreateValueFile (`desfire_create_value_file`)
Creates a value file in a selected DESFire application after authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 20. DESFire CreateLinearRecordFile (`desfire_create_linear_record_file`)
Creates a linear record file in a selected DESFire application after authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 21. DESFire CreateCyclicRecordFile (`desfire_create_cyclic_record_file`)
Creates a cyclic record file in a selected DESFire application after authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 22. DESFire Value Operations (`desfire_value_operations`)
Executes `GetValue`, `Credit`, `Debit`, or `LimitedCredit` on a selected value file.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 23. DESFire CommitTransaction (`desfire_commit_transaction`)
Commits pending transactional updates in the selected application using `CommitTransaction`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 24. DESFire ReadRecords + WriteRecord (`desfire_record_operations`)
Executes record file operations using `ReadRecords` and `WriteRecord`, with optional `CommitTransaction`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 25. DESFire DeleteFile (`desfire_delete_file`)
Deletes one file in the selected application using `DeleteFile`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 26. DESFire ClearRecordFile (`desfire_clear_record_file`)
Clears a linear/cyclic record file using `ClearRecordFile`, with optional `CommitTransaction`.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 27. DESFire FreeMemory (`desfire_free_memory`)
Reads remaining PICC free memory using `FreeMemory` (INS `0x6E`), with optional authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 28. DESFire FormatPICC (`desfire_format_picc`)
Formats (erases) the PICC using `FormatPICC` (INS `0xFC`), with optional authenticate.

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 29. DESFire ChangeFileSettings (`desfire_change_file_settings`)
Updates file communication mode and access rights using `ChangeFileSettings` (INS `0x5F`).

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

### 30. DESFire Scripted Demo (`desfire_scripted_demo`)
PowerShell-only end-to-end demo that chains existing DESFire command executables (no C++ source in this example).

**Files:**
- `run_demo.ps1` - Scripted orchestrator for multi-application demo flow
- `README.md` - Usage, safety notes, and parameter reference
- `STEPS.md` - Step-by-step command plan

### 31. DESFire Session Drift (`desfire_session_drift`)
Long-session drift test runner that creates/uses multiple apps and executes mixed secure operations in two modes:
`drift` (single auth per app workload) and `baseline` (re-auth per operation).

**Files:**
- `main.cpp` - Example implementation
- `CMakeLists.txt` - Build configuration
- `README.md` - Usage and argument examples
- `build_example.ps1` / `build_example.bat` - Build scripts for this example target

## Adding Your Own Example

To create a new example:

1. Create a new directory under `examples/`
2. Create a `main.cpp` file with your example code
3. Create a `CMakeLists.txt` file:

```cmake
add_executable(my_example_name main.cpp)

target_link_libraries(my_example_name
    PRIVATE
        NfcCpp::NfcCpp
)

set_target_properties(my_example_name PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
```

4. Add your example to `examples/CMakeLists.txt`:

```cmake
add_subdirectory(my_example_name)
```

## Notes

- All examples link against the NfcCpp library using the `NfcCpp::NfcCpp` target
- Examples are template code - you'll need to implement the actual logic based on your hardware setup
- Make sure your hardware (PN532 reader, serial ports, etc.) is properly connected before running examples
