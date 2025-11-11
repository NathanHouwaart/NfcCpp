/**
 * @file InListPassiveTarget.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InListPassiveTarget command for card detection
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "Pn532/IPn532Command.h"
#include <etl/vector.h>
#include <cstdint>

namespace pn532
{
    /**
     * @brief Card target types
     * 
     */
    enum class CardTargetType : uint8_t
    {
        TypeA_106kbps = 0x00,
        FeliCa_212kbps = 0x01,
        FeliCa_424kbps = 0x02,
        TypeB_106kbps = 0x03,
        Jewel_106kbps = 0x04
    };

    /**
     * @brief Target information structure
     * 
     */
    struct TargetInfo
    {
        etl::vector<uint8_t, 10> uid;
        uint16_t atqa;
        uint8_t sak;
        etl::vector<uint8_t, 32> ats;

        TargetInfo() : atqa(0), sak(0) {}
    };

    /**
     * @brief InListPassiveTarget options
     * 
     */
    struct InListPassiveTargetOptions
    {
        uint8_t maxTargets;
        CardTargetType target;

        InListPassiveTargetOptions() 
            : maxTargets(1), target(CardTargetType::TypeA_106kbps) {}
    };

    /**
     * @brief InListPassiveTarget command
     * 
     */
    class InListPassiveTarget : public IPn532Command
    {
    public:
        explicit InListPassiveTarget(const InListPassiveTargetOptions& opts);

        etl::string_view name() const override;
        CommandRequest buildRequest() override;
        etl::expected<CommandResponse, error::Error> parseResponse(const Pn532ResponseFrame& frame) override;
        bool expectsDataFrame() const override;

        /**
         * @brief Get detected targets
         * 
         * @return const etl::ivector<TargetInfo>& Vector of detected targets
         */
        const etl::ivector<TargetInfo>& getDetectedTargets() const;

    private:
        InListPassiveTargetOptions options;
        etl::vector<TargetInfo, 2> detectedTargets;

        bool parseTarget(const Pn532ResponseFrame& frame, size_t& index);
        bool parseTypeATarget(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo);
        bool parseUid(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo);
        void parseATS(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo);
        bool parseOtherTarget(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo);
        void populateResponsePayload(CommandResponse& result, uint8_t targetNumber, const TargetInfo& targetInfo);
    };

} // namespace pn532
