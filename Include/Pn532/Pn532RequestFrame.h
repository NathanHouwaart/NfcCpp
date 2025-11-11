/**
 * @file Pn532RequestFrame.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief PN532 request frame builder
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <etl/vector.h>
#include <etl/expected.h>
#include <cstdint>
#include "CommandRequest.h"
#include "Error/Error.h"
#include "Nfc/BufferSizes.h"

namespace pn532
{
    /**
     * @brief PN532 request frame structure
     * 
     * Frame format:
     * - Preamble: 0x00
     * - Start codes: 0x00 0xFF
     * - Length: LEN (1 byte)
     * - Length checksum: LCS = ~LEN + 1
     * - TFI: 0xD4 (host to PN532)
     * - Command code: 1 byte
     * - Data: variable
     * - Data checksum: DCS = ~(TFI + CMD + Data) + 1
     * - Postamble: 0x00
     */
    class Pn532RequestFrame
    {
    public:
        /**
         * @brief Build a PN532 frame from a command request
         * 
         * @param request The command request to frame
         * @return etl::expected<etl::vector<uint8_t, nfc::buffer::PN532_FRAME_MAX>, error::Error> 
         *         Framed data ready for transmission, or error
         */
        static etl::expected<etl::vector<uint8_t, nfc::buffer::PN532_FRAME_MAX>, error::Error> 
            build(const CommandRequest& request);

        /**
         * @brief Build ACK frame
         * 
         * ACK format: 0x00 0x00 0xFF 0x00 0xFF 0x00
         * 
         * @return etl::vector<uint8_t, 6> ACK frame
         */
        static etl::vector<uint8_t, 6> buildAck();

        /**
         * @brief Build NACK frame
         * 
         * NACK format: 0x00 0x00 0xFF 0xFF 0x00 0x00
         * 
         * @return etl::vector<uint8_t, 6> NACK frame
         */
        static etl::vector<uint8_t, 6> buildNack();

    private:
        // Frame protocol constants
        static constexpr uint8_t PREAMBLE = 0x00;
        static constexpr uint8_t START_CODE_1 = 0x00;
        static constexpr uint8_t START_CODE_2 = 0xFF;
        static constexpr uint8_t TFI_HOST_TO_PN532 = 0xD4;
        static constexpr uint8_t POSTAMBLE = 0x00;

        /**
         * @brief Calculate checksum for a data segment
         * 
         * @param data Data to checksum
         * @return uint8_t Checksum value (~sum + 1)
         */
        static uint8_t calculateChecksum(const etl::ivector<uint8_t>& data);

        /**
         * @brief Calculate length checksum (LCS)
         * 
         * @param length Length value
         * @return uint8_t Length checksum (~length + 1)
         */
        static uint8_t calculateLengthChecksum(uint8_t length);
    };

} // namespace pn532
