#pragma once

#include "CommonUtils.hpp"

// Declarations
namespace LLTCAlwaysOnUSB {
    inline bool IsSupported()noexcept;
    inline std::expected<AlwaysOnUSBState, ResultState> GetState()noexcept;
    inline std::expected<void, ResultState> SetState(AlwaysOnUSBState state)noexcept;
}

// Definitions
namespace LLTCAlwaysOnUSB {
    namespace{
        constexpr DWORD IOCTL_ENERGY_SETTINGS = 0x831020E8;
    }

    inline std::expected<AlwaysOnUSBState, ResultState> GetState() noexcept {
        uint32_t input = 0x2;
        uint32_t output = 0;
        
        if (!LLTCCommonUtils::EnergyDrvIoControl(IOCTL_ENERGY_SETTINGS, input, output))
            return std::unexpected(ResultState::Failed);
        
        uint32_t state = LLTCCommonUtils::ReverseEndianness(output);

        if (!LLTCCommonUtils::GetNthBit(state, 31))
            return AlwaysOnUSBState::Off;
        if (LLTCCommonUtils::GetNthBit(state, 23))
            return AlwaysOnUSBState::OnAlways;
        return AlwaysOnUSBState::OnWhenSleeping;
    }
    
    inline std::expected<void, ResultState> SetState(AlwaysOnUSBState state) noexcept {
        std::vector<uint32_t> commands;
        switch (state) {
            case AlwaysOnUSBState::Off:             commands = {0xB, 0x12}; break;
            case AlwaysOnUSBState::OnWhenSleeping:  commands = {0xA, 0x12}; break;
            case AlwaysOnUSBState::OnAlways:        commands = {0xA, 0x13}; break;
            default: return std::unexpected(ResultState::InvalidParameter);
        }
        
        for (uint32_t cmd : commands) {
            uint32_t dummyOutput = 0;
            if (!LLTCCommonUtils::EnergyDrvIoControl(IOCTL_ENERGY_SETTINGS, cmd, dummyOutput))
                return std::unexpected(ResultState::Failed);
        }
        
        for (int retry = 0; retry < 10; ++retry) {
            auto currentState = GetState();
            if(!currentState)
                return std::unexpected(currentState.error());
            if (currentState.value() == state)
                return {};
            Sleep(50);
        }
        
        return std::unexpected(ResultState::RetryTimeout);
    }
    
    inline bool IsSupported() noexcept {
        return GetState().has_value();
    }
}