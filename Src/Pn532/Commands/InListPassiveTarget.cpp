/**
 * @file InListPassiveTarget.cpp
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief InListPassiveTarget command implementation
 * @version 0.1
 * @date 2025-11-11
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "Pn532/Commands/InListPassiveTarget.h"
#include "Error/Pn532Error.h"

using namespace error;

namespace pn532
{
    InListPassiveTarget::InListPassiveTarget(const InListPassiveTargetOptions& opts)
        : options(opts)
    {
    }

    etl::string_view InListPassiveTarget::name() const
    {
        return "InListPassiveTarget";
    }

    CommandRequest InListPassiveTarget::buildRequest()
    {
        // Build payload: [MaxTg] [BrTy]
        etl::vector<uint8_t, 3> payload;
        payload.push_back(options.maxTargets);
        payload.push_back(static_cast<uint8_t>(options.target));
        return createCommandRequest(0x4A, payload); // 0x4A = InListPassiveTarget
    }

    etl::expected<CommandResponse, Error> InListPassiveTarget::parseResponse(const Pn532ResponseFrame& frame)
    {
        detectedTargets.clear();
        
        const auto& data = frame.data();
        
        if (data.empty())
        {
            return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
        }

        // First byte is number of targets found
        uint8_t nbTargets = data[0];
        
        size_t index = 1;
        for (uint8_t i = 0; i < nbTargets; ++i)
        {
            if (!parseTarget(frame, index))
            {
                return etl::unexpected(Error::fromPn532(Pn532Error::InvalidResponse));
            }
        }

        return createCommandResponse(frame.getCommandCode(), data);
    }

    bool InListPassiveTarget::expectsDataFrame() const
    {
        return true;
    }

    const etl::ivector<TargetInfo>& InListPassiveTarget::getDetectedTargets() const
    {
        return detectedTargets;
    }

    bool InListPassiveTarget::parseTarget(const Pn532ResponseFrame& frame, size_t& index)
    {
        const auto& data = frame.data();
        
        if (index >= data.size())
        {
            return false;
        }

        // Skip target number
        index++;
        
        TargetInfo targetInfo;
        
        bool success = (options.target == CardTargetType::TypeA_106kbps)
            ? parseTypeATarget(frame, index, targetInfo)
            : parseOtherTarget(frame, index, targetInfo);

        if (!success)
        {
            return false;
        }

        detectedTargets.push_back(targetInfo);
        return true;
    }

    bool InListPassiveTarget::parseTypeATarget(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo)
    {
        const auto& data = frame.data();
        
        // Format: [ATQA(2)][SAK(1)][UIDLen(1)][UID...][ATSLen(1)][ATS...]
        if (index + 4 > data.size())
        {
            return false;
        }

        // ATQA (SENS_RES) - 2 bytes, little endian
        targetInfo.atqa = static_cast<uint16_t>(data[index]) |
                         (static_cast<uint16_t>(data[index + 1]) << 8);
        index += 2;

        // SAK (SEL_RES)
        targetInfo.sak = data[index++];

        // UID
        if (!parseUid(frame, index, targetInfo))
        {
            return false;
        }

        // ATS (optional)
        parseATS(frame, index, targetInfo);
        
        return true;
    }

    bool InListPassiveTarget::parseUid(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo)
    {
        const auto& data = frame.data();
        
        if (index >= data.size())
        {
            return false;
        }

        uint8_t uidLength = data[index++];
        
        if (index + uidLength > data.size())
        {
            return false;
        }

        targetInfo.uid.assign(data.begin() + index, data.begin() + index + uidLength);
        index += uidLength;
        
        return true;
    }

    void InListPassiveTarget::parseATS(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo)
    {
        const auto& data = frame.data();
        
        // Only parse ATS if card supports ISO-DEP (SAK bit 5 set) and data is available
        if (index >= data.size() || (targetInfo.sak & 0x20) == 0)
        {
            return;
        }

        // ATS length byte (TL) includes itself in the count per ISO 14443-4
        uint8_t atsLength = data[index++];
        uint8_t dataBytes = (atsLength > 0) ? (atsLength - 1) : 0;
        
        if (dataBytes > 0 && index + dataBytes <= data.size())
        {
            targetInfo.ats.assign(data.begin() + index, data.begin() + index + dataBytes);
            index += dataBytes;
        }
    }

    bool InListPassiveTarget::parseOtherTarget(const Pn532ResponseFrame& frame, size_t& index, TargetInfo& targetInfo)
    {
        const auto& data = frame.data();
        
        if (index + 1 > data.size())
        {
            return false;
        }

        uint8_t dataLength = data[index++];
        
        if (index + dataLength > data.size())
        {
            return false;
        }

        targetInfo.uid.assign(data.begin() + index, data.begin() + index + dataLength);
        index += dataLength;
        
        return true;
    }

} // namespace pn532
