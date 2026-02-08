#pragma once

#include "CommonUtils.hpp"

// Declarations
namespace LLTCWhiteKeyboardBacklight{
    inline std::expected<WhiteKeyboardBacklightState, ResultState> GetState() noexcept;
    inline std::expected<void, ResultState> SetState(WhiteKeyboardBacklightState state) noexcept;
}

// Definitions
namespace LLTCWhiteKeyboardBacklight {
    namespace{
        constexpr DWORD IOCTL_ENERGY_KEYBOARD = 0x83102144;

        class DriverHandleSingleton {
            public:
                static HANDLE Get() {
                    static DriverHandleSingleton instance;
                    return instance.handle_;
                }
                ~DriverHandleSingleton() {
                    if (handle_ != INVALID_HANDLE_VALUE) {
                        CloseHandle(handle_);
                    }
                }
            private:
                DriverHandleSingleton() : handle_(LLTCCommonUtils::GetEnergyDriverHandle()) {}
                HANDLE handle_ = INVALID_HANDLE_VALUE;
        };
        inline bool ExecuteKeyboardIoctl(uint32_t inBuffer, uint32_t& outBuffer) {
            HANDLE hDriver = DriverHandleSingleton::Get();
            if (hDriver == INVALID_HANDLE_VALUE) {
                return false;
            }

            DWORD bytesReturned = 0;
            return DeviceIoControl(
                hDriver,
                IOCTL_ENERGY_KEYBOARD,
                &inBuffer, sizeof(inBuffer),
                &outBuffer, sizeof(outBuffer),
                &bytesReturned,
                nullptr
            ) && (bytesReturned == sizeof(uint32_t));
        }
        
        inline bool IsSupported() { // maybe some bug
            uint32_t outBuffer = 0;
            if(!ExecuteKeyboardIoctl(0x1, outBuffer))
                return false;
            return ((outBuffer >> 1) == 0x2);
        }
    }

    inline std::expected<WhiteKeyboardBacklightState, ResultState> GetState() noexcept {
        uint32_t outBuffer = 0;
        if(!ExecuteKeyboardIoctl(0x22, outBuffer))
            return std::unexpected(ResultState::Failed);
        HANDLE hDriver = LLTCCommonUtils::GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return std::unexpected(ResultState::Failed);
        }
        switch (outBuffer) {
            case 0x1: return WhiteKeyboardBacklightState::Off;
            case 0x3: return WhiteKeyboardBacklightState::Low;
            case 0x5: return WhiteKeyboardBacklightState::High;
            default: return std::unexpected(ResultState::Failed);
        }
    }

    inline std::expected<void, ResultState> SetState(WhiteKeyboardBacklightState newState) noexcept {
        uint32_t command = 0;
        switch (newState) {
            case WhiteKeyboardBacklightState::Off:  command = 0x00023; break;
            case WhiteKeyboardBacklightState::Low:  command = 0x10023; break;
            case WhiteKeyboardBacklightState::High: command = 0x20023; break;
            default: return std::unexpected(ResultState::InvalidParameter);
        }

        uint32_t dummyOutput = 0;
        if(!ExecuteKeyboardIoctl(command, dummyOutput))
            return std::unexpected(ResultState::Failed);

        const int MAX_RETRIES = 100;
        const int DELAY_MS = 50;
        for (int i = 0; i < MAX_RETRIES; ++i) {
            auto currentState = GetState();
            if (currentState.has_value() && currentState.value() == newState) {
                return {};
            }
            Sleep(DELAY_MS);
        }
        
        return std::unexpected(ResultState::RetryTimeout);
    }
}