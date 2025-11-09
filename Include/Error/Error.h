/**
 * @file Error.h
 * @author Nathan Houwaart (n.m.houwaart@hva.nl)
 * @brief 
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include "HardwareError.h"
#include "LinkError.h"
#include "Pn532Error.h"
#include "Rc522Error.h"
#include "CardManagerError.h"
#include "ApduError.h"
#include "DesfireError.h"

#include <etl/variant.h>
#include <etl/string_view.h>
#include <etl/string.h>

namespace error {

    enum class ErrorLayer : uint8_t {
        Hardware,
        Link,
        Pn532,
        Rc522,
        CardManager,
        Apdu,
        Desfire
        // MifareClassic -> Future
        // Ultralight
        // Felica
    };


    class Error {
        public:

            using ErrorVariant = etl::variant<
                HardwareError, 
                LinkError, 
                Pn532Error, 
                Rc522Error, 
                CardManagerError, 
                ApduError, 
                DesfireError
            >;

            Error(ErrorLayer layer, ErrorVariant errorCode)
                : layer(layer), errorCode(errorCode) {}

            static Error fromHardware(HardwareError err) {
                return Error{ErrorLayer::Hardware, err};
            }

            static Error fromPn532(Pn532Error err) {
                return Error{ErrorLayer::Pn532, err};
            }

            static Error fromLink(LinkError err) {
                return Error{ErrorLayer::Link, err};
            }

            static Error fromRc522(Rc522Error err) {
                return Error{ErrorLayer::Rc522, err};
            }

            static Error fromCardManager(CardManagerError err) {
                return Error{ErrorLayer::CardManager, err};
            }

            static Error fromApdu(ApduError err) {
                return Error{ErrorLayer::Apdu, err};
            }

            static Error fromDesfire(DesfireError err) {
                return Error{ErrorLayer::Desfire, err};
            }

            template<typename T>
            bool is() const {
                return etl::holds_alternative<T>(errorCode);
            }

            template<typename T>
            T get() const {
                return etl::get<T>(errorCode);
            }

            etl::string_view layerName(ErrorLayer layer) const {
                switch (layer) {
                    case ErrorLayer::Hardware:
                        return "Hardware";
                    case ErrorLayer::Link:
                        return "Link";
                    case ErrorLayer::Pn532:
                        return "PN532";
                    case ErrorLayer::Rc522:
                        return "RC522";
                    case ErrorLayer::CardManager:
                        return "CardManager";
                    case ErrorLayer::Apdu:
                        return "APDU";
                    case ErrorLayer::Desfire:
                        return "Desfire";
                    default:
                        return "Unknown";
                }
            }

            etl::string_view nameOf(HardwareError err) const {
                switch (err) {
                    case HardwareError::Ok:
                        return "Ok";
                    case HardwareError::Timeout:
                        return "Timeout";
                    case HardwareError::Nack:
                        return "Nack";
                    case HardwareError::BusError:
                        return "BusError";
                    case HardwareError::BufferOverflow:
                        return "BufferOverflow";
                    case HardwareError::DeviceNotFound:
                        return "DeviceNotFound";
                    case HardwareError::WriteFailed:
                        return "WriteFailed";
                    case HardwareError::ReadFailed:
                        return "ReadFailed";
                    case HardwareError::InvalidConfiguration:
                        return "InvalidConfiguration";
                    case HardwareError::NotSupported:
                        return "NotSupported";
                    case HardwareError::Unknown:
                        return "UnknownError";
                    default:
                        return "UndefinedHardwareError";
                }
            }

            etl::string_view nameOf(LinkError err) const {
                switch (err) {
                    case LinkError::Ok:
                        return "Success";
                    case LinkError::Timeout:
                        return "Timeout";
                    case LinkError::CrcError:
                        return "CrcError";
                    case LinkError::ParityError:
                        return "ParityError";
                    case LinkError::Collision:
                        return "Collision";
                    case LinkError::BufferInsufficient:
                        return "BufferInsufficient";
                    case LinkError::RfError:
                        return "RfError";
                    case LinkError::AuthenticationError:
                        return "AuthenticationError";
                    case LinkError::CardDisappeared:
                        return "CardDisappeared";
                    default:
                        return "UndefinedLinkError";
                }
            }

            etl::string_view nameOf(Pn532Error err) const {
                switch (err) {
                    case Pn532Error::Ok:
                        return "OK";
                    case Pn532Error::Timeout:
                        return "Timeout";
                    case Pn532Error::CRCError:
                        return "CRCError";
                    case Pn532Error::ParityError:
                        return "ParityError";
                    case Pn532Error::CollisionError:
                        return "CollisionError";
                    case Pn532Error::MifareFramingError:
                        return "MifareFramingError";
                    case Pn532Error::BufferSizeInsufficient:
                        return "BufferSizeInsufficient";
                    case Pn532Error::SelftestFail:
                        return "SelftestFail";
                    case Pn532Error::RFBufferOverflow:
                        return "RFBufferOverflow";
                    case Pn532Error::RFFieldTimeout:
                        return "RFFieldTimeout";
                    case Pn532Error::RFProtocolError:
                        return "RFProtocolError";
                    case Pn532Error::InvalidAckFrame:
                        return "InvalidAckFrame";
                    case Pn532Error::TemperatureError:
                        return "TemperatureError";
                    case Pn532Error::InternalBufferOverflow:
                        return "InternalBufferOverflow";
                    case Pn532Error::InvalidParameter:
                        return "InvalidParameter";
                    case Pn532Error::MifareAutError:
                        return "MifareAutError";
                    case Pn532Error::UIDCheckByteError:
                        return "UIDCheckByteError";
                    case Pn532Error::WrongConfig:
                        return "WrongConfig";
                    case Pn532Error::WrongCommand:
                        return "WrongCommand";
                    case Pn532Error::Released:
                        return "Released";
                    case Pn532Error::OverCurrent:
                        return "OverCurrent";
                    case Pn532Error::MissingDEP:
                        return "MissingDEP";
                    case Pn532Error::SAMerror:
                        return "SAMerror";
                    case Pn532Error::FrameCheckFailed:
                        return "FrameCheckFailed";
                    case Pn532Error::InvalidResponse:
                        return "InvalidResponse";
                    default:
                        return "UndefinedPn532Error";
                }
            }

            etl::string_view nameOf(Rc522Error err) const {
                switch (err) {
                    case Rc522Error::Ok:
                        return "Ok";
                    case Rc522Error::Timeout:
                        return "Timeout";
                    case Rc522Error::FifoOverflow:
                        return "FifoOverflow";
                    case Rc522Error::CrcError:
                        return "CrcError";
                    case Rc522Error::ParityError:
                        return "ParityError";
                    case Rc522Error::Collision:
                        return "Collision";
                    case Rc522Error::AuthError:
                        return "AuthError";
                    case Rc522Error::FrameError:
                        return "FrameError";
                    case Rc522Error::ProtocolError:
                        return "ProtocolError";
                    default:
                        return "UndefinedRc522Error";
                }
            }

            etl::string_view nameOf(CardManagerError err) const {
                switch (err) {
                    case CardManagerError::Ok:
                        return "Ok";
                    case CardManagerError::NoCardPresent:
                        return "NoCardPresent";
                    case CardManagerError::MultipleCards:
                        return "MultipleCards";
                    case CardManagerError::UnsupportedCardType:
                        return "UnsupportedCardType";
                    case CardManagerError::CardMute:
                        return "CardMute";
                    case CardManagerError::AuthenticationRequired:
                        return "AuthenticationRequired";
                    case CardManagerError::OperationFailed:
                        return "OperationFailed";
                    case CardManagerError::InvalidParameter:
                        return "InvalidParameter";
                    default:
                        return "UndefinedCardManagerError";
                }
            }

            etl::string_view nameOf(ApduError err) const {
                switch (err) {
                    case ApduError::Ok:
                        return "Ok";
                    case ApduError::WrongLength:
                        return "WrongLength";
                    case ApduError::SecurityStatusNotSatisfied:
                        return "SecurityStatusNotSatisfied";
                    case ApduError::ConditionsNotSatisfied:
                        return "ConditionsNotSatisfied";
                    case ApduError::FileNotFound:
                        return "FileNotFound";
                    case ApduError::WrongP1P2:
                        return "WrongP1P2";
                    case ApduError::Unknown:
                        return "Unknown";
                    default:
                        return "UndefinedApduError";
                }
            }

            etl::string_view nameOf(DesfireError err) const {
                switch (err) {
                    case DesfireError::Ok:
                        return "Ok";
                    case DesfireError::NoChanges:
                        return "NoChanges";
                    case DesfireError::OutOfEeprom:
                        return "OutOfEeprom";
                    case DesfireError::IllegalCommand:
                        return "IllegalCommand";
                    case DesfireError::IntegrityError:
                        return "IntegrityError";
                    case DesfireError::NoSuchKey:
                        return "NoSuchKey";
                    case DesfireError::LengthError:
                        return "LengthError";
                    case DesfireError::PermissionDenied:
                        return "PermissionDenied";
                    case DesfireError::ParameterError:
                        return "ParameterError";
                    case DesfireError::ApplicationNotFound:
                        return "ApplicationNotFound";
                    case DesfireError::ApplIntegrityError:
                        return "ApplIntegrityError";
                    case DesfireError::AuthenticationError:
                        return "AuthenticationError";
                    case DesfireError::AdditionalFrame:
                        return "AdditionalFrame";
                    case DesfireError::BoundaryError:
                        return "BoundaryError";
                    case DesfireError::PiccIntegrityError:
                        return "PiccIntegrityError";
                    case DesfireError::CommandAborted:
                        return "CommandAborted";
                    case DesfireError::PiccDisabled:
                        return "PiccDisabled";
                    case DesfireError::CountError:
                        return "CountError";
                    case DesfireError::DuplicateError:
                        return "DuplicateError";
                    case DesfireError::EepromError:
                        return "EepromError";
                    case DesfireError::FileNotFound:
                        return "FileNotFound";
                    case DesfireError::FileIntegrityError:
                        return "FileIntegrityError";
                    default:
                        return "UndefinedDesfireError";
                }
            }

            etl::string<160> toString() const {
                etl::string<160> result;
                auto layer_name = layerName(layer);
                result.assign(layer_name.begin(), layer_name.end());
                result.append(" Error: ");
                
                auto error_name = etl::visit([this](auto&& arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, HardwareError>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, LinkError>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, Pn532Error>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, Rc522Error>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, CardManagerError>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, ApduError>) {
                            return nameOf(arg);
                        } else if constexpr (std::is_same_v<T, DesfireError>) {
                            return nameOf(arg);
                        } else {
                            return "Unknown Error Type";
                        }
                    }, errorCode);
                
                result.append(error_name.begin(), error_name.end());
                return result;
            }
        
        private:
            ErrorLayer   layer;
            ErrorVariant errorCode;

    };

} // namespace error