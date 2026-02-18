/**
 * @file DesfireCard.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief DESFire card implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <cstdint>
#include <etl/array.h>
#include <etl/vector.h>
#include <etl/expected.h>
#include "DesfireContext.h"
#include "DesfireAuthMode.h"
#include "DesfireKeyType.h"
#include "Error/Error.h"

namespace nfc
{
    /**
     * @brief Parsed DESFire file settings
     */
    struct DesfireFileSettingsInfo
    {
        uint8_t fileNo = 0U;
        uint8_t fileType = 0xFFU;
        uint8_t communicationSettings = 0x00U;

        uint8_t readAccess = 0x0FU;
        uint8_t writeAccess = 0x0FU;
        uint8_t readWriteAccess = 0x0FU;
        uint8_t changeAccess = 0x0FU;

        bool hasFileSize = false;
        uint32_t fileSize = 0U;

        bool hasValueSettings = false;
        uint32_t lowerLimit = 0U;
        uint32_t upperLimit = 0U;
        uint32_t limitedCreditValue = 0U;
        bool limitedCreditEnabled = false;
        bool freeGetValueEnabled = false;

        bool hasRecordSettings = false;
        uint32_t recordSize = 0U;
        uint32_t maxRecords = 0U;
        uint32_t currentRecords = 0U;
    };

    // Forward declarations
    class ISecurePipe;
    class PlainPipe;
    class MacPipe;
    class EncPipe;
    class IDesfireCommand;
    struct DesfireRequest;
    struct DesfireResult;
    class IApduTransceiver;
    class IWire;

    /**
     * @brief DESFire card class
     * 
     * Manages DESFire card operations with different security pipes
     */
    class DesfireCard
    {
    public:
        static constexpr size_t MAX_DATA_IO_SIZE = 4096U;

        /**
         * @brief Construct a new DesfireCard
         * 
         * @param transceiver APDU transceiver
         * @param wire Wire strategy for APDU framing (managed by CardManager)
         */
        DesfireCard(IApduTransceiver& transceiver, IWire& wire);

        /**
         * @brief Get the DESFire context (read-only)
         *
         * Intended for diagnostics and tracing in example tooling.
         *
         * @return const DesfireContext& Reference to the current session context
         */
        const DesfireContext& getContext() const;

        /**
         * @brief Execute a DESFire command
         * 
         * @param command Command to execute
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> executeCommand(IDesfireCommand& command);

        /**
         * @brief Select application
         * 
         * @param aid Application ID (3 bytes)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> selectApplication(const etl::array<uint8_t, 3>& aid);

        /**
         * @brief Authenticate with the card
         * 
         * @param keyNo Key number
         * @param key Key data
         * @param mode Authentication mode
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> authenticate(
            uint8_t keyNo,
            const etl::vector<uint8_t, 24>& key,
            DesfireAuthMode mode);

        /**
         * @brief Create a DESFire application
         *
         * @param aid Target application AID (3 bytes)
         * @param keySettings1 Application master key settings byte
         * @param keyCount Number of keys in the application (1..14)
         * @param keyType Application key type (DES/2K3DES/3K3DES/AES)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createApplication(
            const etl::array<uint8_t, 3>& aid,
            uint8_t keySettings1,
            uint8_t keyCount,
            DesfireKeyType keyType);

        /**
         * @brief Delete a DESFire application
         *
         * @param aid Target application AID (3 bytes)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> deleteApplication(const etl::array<uint8_t, 3>& aid);

        /**
         * @brief Create a standard data file in the currently selected application
         *
         * Runs CreateStdDataFile (INS 0xCD).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings File communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param fileSize File size in bytes (1..0xFFFFFF)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createStdDataFile(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            uint32_t fileSize);

        /**
         * @brief Create a backup data file in the currently selected application
         *
         * Runs CreateBackupDataFile (INS 0xCB).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings File communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param fileSize File size in bytes (1..0xFFFFFF)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createBackupDataFile(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            uint32_t fileSize);

        /**
         * @brief Create a linear record file in the currently selected application
         *
         * Runs CreateLinearRecordFile (INS 0xC1).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings File communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param recordSize Size of one record in bytes (1..0xFFFFFF)
         * @param maxRecords Maximum number of records (1..0xFFFFFF)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createLinearRecordFile(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            uint32_t recordSize,
            uint32_t maxRecords);

        /**
         * @brief Create a cyclic record file in the currently selected application
         *
         * Runs CreateCyclicRecordFile (INS 0xC0).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings File communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param recordSize Size of one record in bytes (1..0xFFFFFF)
         * @param maxRecords Maximum number of records in ring buffer (1..0xFFFFFF)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createCyclicRecordFile(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            uint32_t recordSize,
            uint32_t maxRecords);

        /**
         * @brief Create a value file in the currently selected application
         *
         * Runs CreateValueFile (INS 0xCC).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings File communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param lowerLimit Lower bound (signed 32-bit)
         * @param upperLimit Upper bound (signed 32-bit)
         * @param limitedCreditValue Limited-credit configured value (signed 32-bit)
         * @param limitedCreditEnabled Enables limited credit when true
         * @param freeGetValueEnabled Enables free get value when true
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> createValueFile(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            int32_t lowerLimit,
            int32_t upperLimit,
            int32_t limitedCreditValue,
            bool limitedCreditEnabled,
            bool freeGetValueEnabled = false);

        /**
         * @brief Delete one file in the currently selected application
         *
         * Runs DeleteFile (INS 0xDF).
         *
         * @param fileNo File number (0..31)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> deleteFile(uint8_t fileNo);

        /**
         * @brief Change settings of one file in the currently selected application
         *
         * Runs ChangeFileSettings (INS 0x5F).
         *
         * @param fileNo File number (0..31)
         * @param communicationSettings New file communication mode (0x00 plain, 0x01 MAC, 0x03 enciphered)
         * @param readAccess Read access key selector nibble (0x0..0xF)
         * @param writeAccess Write access key selector nibble (0x0..0xF)
         * @param readWriteAccess Read+write access key selector nibble (0x0..0xF)
         * @param changeAccess Change access rights key selector nibble (0x0..0xF)
         * @param commandCommunicationSettings How command is protected (0x00 plain, 0x01 MAC, 0x03 enc, 0xFF auto)
         * @param authMode Authentication mode hint for session-cipher handling
         * @param sessionKeyType Optional explicit session key type override
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> changeFileSettings(
            uint8_t fileNo,
            uint8_t communicationSettings,
            uint8_t readAccess,
            uint8_t writeAccess,
            uint8_t readWriteAccess,
            uint8_t changeAccess,
            uint8_t commandCommunicationSettings = 0xFFU,
            DesfireAuthMode authMode = DesfireAuthMode::ISO,
            DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN);

        /**
         * @brief Clear a linear/cyclic record file in the currently selected application
         *
         * Runs ClearRecordFile (INS 0xEB).
         *
         * Note: this operation is transactional on DESFire and usually needs
         * CommitTransaction to persist.
         *
         * @param fileNo File number (0..31)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> clearRecordFile(uint8_t fileNo);

        /**
         * @brief Get current value from a value file
         *
         * Runs GetValue (INS 0x6C).
         *
         * @param fileNo File number (0..31)
         * @return etl::expected<int32_t, error::Error> Current signed 32-bit value or error
         */
        etl::expected<int32_t, error::Error> getValue(uint8_t fileNo);

        /**
         * @brief Apply credit operation on a value file
         *
         * Runs Credit (INS 0x0C).
         *
         * @param fileNo File number (0..31)
         * @param value Credit amount (must be >= 0)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> credit(uint8_t fileNo, int32_t value);

        /**
         * @brief Apply debit operation on a value file
         *
         * Runs Debit (INS 0xDC).
         *
         * @param fileNo File number (0..31)
         * @param value Debit amount (must be >= 0)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> debit(uint8_t fileNo, int32_t value);

        /**
         * @brief Apply limited credit operation on a value file
         *
         * Runs LimitedCredit (INS 0x1C).
         *
         * @param fileNo File number (0..31)
         * @param value Limited-credit amount (must be >= 0)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> limitedCredit(uint8_t fileNo, int32_t value);

        /**
         * @brief Commit pending transactional changes
         *
         * Runs CommitTransaction (INS 0xC7).
         *
         * Applies pending updates from transactional operations, such as:
         * - writes to backup data files
         * - credit/debit/limited-credit changes on value files
         *
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> commitTransaction();

        /**
         * @brief Read data bytes from a standard/backup data file
         *
         * Runs ReadData (INS 0xBD) in chunked mode.
         *
         * @param fileNo File number (0..31)
         * @param offset Start offset (24-bit)
         * @param length Number of bytes to read (1..MAX_DATA_IO_SIZE)
         * @param chunkSize Max bytes per command cycle (0 uses default)
         * @return etl::expected<etl::vector<uint8_t, MAX_DATA_IO_SIZE>, error::Error> Data or error
         */
        etl::expected<etl::vector<uint8_t, MAX_DATA_IO_SIZE>, error::Error> readData(
            uint8_t fileNo,
            uint32_t offset,
            uint32_t length,
            uint16_t chunkSize = 0U);

        /**
         * @brief Write data bytes to a standard/backup data file
         *
         * Runs WriteData (INS 0x3D) in chunked mode.
         *
         * @param fileNo File number (0..31)
         * @param offset Start offset (24-bit)
         * @param data Data bytes to write
         * @param chunkSize Max bytes per command cycle (0 uses default)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> writeData(
            uint8_t fileNo,
            uint32_t offset,
            const etl::ivector<uint8_t>& data,
            uint16_t chunkSize = 0U);

        /**
         * @brief Read records from a linear/cyclic record file
         *
         * Runs ReadRecords (INS 0xBB).
         *
         * Uses record offset/count command fields.
         *
         * @param fileNo File number (0..31)
         * @param recordOffset Record offset (24-bit)
         * @param recordCount Number of records to read (0 => all from offset)
         * @param chunkSize Max bytes per command cycle for transport framing (0 uses default)
         * @return etl::expected<etl::vector<uint8_t, MAX_DATA_IO_SIZE>, error::Error> Raw record bytes or error
         */
        etl::expected<etl::vector<uint8_t, MAX_DATA_IO_SIZE>, error::Error> readRecords(
            uint8_t fileNo,
            uint32_t recordOffset,
            uint32_t recordCount,
            uint16_t chunkSize = 0U);

        /**
         * @brief Write records to a linear/cyclic record file
         *
         * Runs WriteRecord (INS 0x3B) in chunked mode.
         *
         * WriteRecord uses byte-based addressing inside the record currently
         * being assembled:
         * - `offset` is a byte offset within the record
         * - `data.size()` is byte length
         *
         * Boundary rule: `offset + data.size()` must not exceed the file
         * record size.
         *
         * @param fileNo File number (0..31)
         * @param offset Byte offset within record (24-bit)
         * @param data Record payload bytes
         * @param chunkSize Max bytes per command cycle (0 uses default)
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> writeRecord(
            uint8_t fileNo,
            uint32_t offset,
            const etl::ivector<uint8_t>& data,
            uint16_t chunkSize = 0U);

        /**
         * @brief Get DESFire version payload bytes
         *
         * Runs GetVersion (INS 0x60) and returns concatenated payload bytes
         * across all response frames.
         *
         * @return etl::expected<etl::vector<uint8_t, 96>, error::Error> Version payload or error
         */
        etl::expected<etl::vector<uint8_t, 96>, error::Error> getVersion();

        /**
         * @brief Format the PICC (erase all applications/files)
         *
         * Runs FormatPICC (INS 0xFC).
         *
         * This is destructive and should only be used intentionally.
         * Select PICC AID `000000` and authenticate PICC master key if
         * required by key settings.
         *
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> formatPicc();

        /**
         * @brief Get remaining free memory on the PICC
         *
         * Runs FreeMemory (INS 0x6E) and returns the reported free byte count
         * as 24-bit little-endian value.
         *
         * Note: This is a PICC-level command. Select PICC AID `000000`
         * before calling it.
         *
         * @return etl::expected<uint32_t, error::Error> Free memory in bytes or error
         */
        etl::expected<uint32_t, error::Error> freeMemory();

        /**
         * @brief Get list of application IDs
         *
         * Runs GetApplicationIDs (INS 0x6A) and returns parsed 3-byte AIDs.
         *
         * @return etl::expected<etl::vector<etl::array<uint8_t, 3>, 84>, error::Error> AID list or error
         */
        etl::expected<etl::vector<etl::array<uint8_t, 3>, 84>, error::Error> getApplicationIds();

        /**
         * @brief Get file IDs for the currently selected application
         *
         * Runs GetFileIDs (INS 0x6F).
         *
         * @return etl::expected<etl::vector<uint8_t, 32>, error::Error> File ID list or error
         */
        etl::expected<etl::vector<uint8_t, 32>, error::Error> getFileIds();

        /**
         * @brief Get settings of one file in the currently selected application
         *
         * Runs GetFileSettings (INS 0xF5).
         *
         * @param fileNo Target file number
         * @return etl::expected<DesfireFileSettingsInfo, error::Error> Parsed file settings or error
         */
        etl::expected<DesfireFileSettingsInfo, error::Error> getFileSettings(uint8_t fileNo);

        /**
         * @brief Get key settings bytes for current selected application/PICC
         *
         * Runs GetKeySettings (INS 0x45).
         *
         * @return etl::expected<etl::array<uint8_t, 2>, error::Error> {KeySettings1, KeySettings2} or error
         */
        etl::expected<etl::array<uint8_t, 2>, error::Error> getKeySettings();

        /**
         * @brief Get key version for one key number
         *
         * Runs GetKeyVersion (INS 0x64).
         *
         * @param keyNo Target key number
         * @return etl::expected<uint8_t, error::Error> Key version byte or error
         */
        etl::expected<uint8_t, error::Error> getKeyVersion(uint8_t keyNo);

        /**
         * @brief Change key settings byte (KeySettings1) for selected PICC/application
         *
         * Runs ChangeKeySettings (INS 0x54).
         *
         * @param keySettings New KeySettings1 byte
         * @param authMode Authentication mode used to derive session cipher semantics
         * @param sessionKeyType Optional explicit session key type override
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> changeKeySettings(
            uint8_t keySettings,
            DesfireAuthMode authMode = DesfireAuthMode::ISO,
            DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN);

        /**
         * @brief Set PICC configuration flags via SetConfiguration
         *
         * Runs SetConfiguration (INS 0x5C, subcommand 0x00).
         *
         * @param piccConfiguration PICC configuration byte (bit0 disable format, bit1 random UID)
         * @param authMode Authentication mode used to derive session cipher semantics
         * @param sessionKeyType Optional explicit session key type override
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> setConfigurationPicc(
            uint8_t piccConfiguration,
            DesfireAuthMode authMode = DesfireAuthMode::ISO,
            DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN);

        /**
         * @brief Set ATS bytes via SetConfiguration
         *
         * Runs SetConfiguration (INS 0x5C, subcommand 0x01).
         *
         * @param ats ATS bytes (typically includes TL as first byte)
         * @param authMode Authentication mode used to derive session cipher semantics
         * @param sessionKeyType Optional explicit session key type override
         * @return etl::expected<void, error::Error> Success or error
         */
        etl::expected<void, error::Error> setConfigurationAts(
            const etl::ivector<uint8_t>& ats,
            DesfireAuthMode authMode = DesfireAuthMode::ISO,
            DesfireKeyType sessionKeyType = DesfireKeyType::UNKNOWN);

        /**
         * @brief Get real card UID
         * 
         * @return etl::expected<etl::vector<uint8_t, 10>, error::Error> UID or error
         */
        etl::expected<etl::vector<uint8_t, 10>, error::Error> getRealCardUid();

        /**
         * @brief Wrap a DESFire request using the appropriate pipe
         * 
         * @param request Request to wrap
         * @return etl::expected<etl::vector<uint8_t, 261>, error::Error> Wrapped data or error
         */
        etl::expected<etl::vector<uint8_t, 261>, error::Error> wrapRequest(const DesfireRequest& request);

        /**
         * @brief Unwrap a response using the appropriate pipe
         * 
         * @param response Response to unwrap
         * @return etl::expected<DesfireResult, error::Error> Unwrapped result or error
         */
        etl::expected<DesfireResult, error::Error> unwrapResponse(const etl::ivector<uint8_t>& response);

    private:
        IApduTransceiver& transceiver;
        DesfireContext context;
        IWire* wire;  // Wire strategy for APDU framing

        PlainPipe* plainPipe;
        MacPipe* macPipe;
        EncPipe* encPipe;
    };

} // namespace nfc
