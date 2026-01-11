#pragma once
#include "CommonUtils.hpp"
#include <cstdint>
#include <optional>
#include <vector>
#include <chrono>
#include <thread>

enum class AlwaysOnUSBState {
    Off,
    OnWhenSleeping,
    OnAlways
};

namespace LenovoFeatures {

    constexpr DWORD IOCTL_ENERGY_SETTINGS = 0x831020E8;

    inline std::optional<AlwaysOnUSBState> GetAlwaysOnUSBState() {
        uint32_t input = 0x2;
        uint32_t output = 0;
        
        if (!LenovoCommonUtils::EnergyDrvIoControl(IOCTL_ENERGY_SETTINGS, input, output)) {
            return std::nullopt;
        }
        
        uint32_t state = LenovoCommonUtils::ReverseEndianness(output);
        
        if (!LenovoCommonUtils::GetNthBit(state, 31)) {
            return AlwaysOnUSBState::Off;
        }
        
        if (LenovoCommonUtils::GetNthBit(state, 23)) {
            return AlwaysOnUSBState::OnAlways;
        }
        
        return AlwaysOnUSBState::OnWhenSleeping;
    }
    
    inline bool SetAlwaysOnUSBState(AlwaysOnUSBState state) {
        std::vector<uint32_t> commands;
        switch (state) {
            case AlwaysOnUSBState::Off:
                commands = {0xB, 0x12};
                break;
            case AlwaysOnUSBState::OnWhenSleeping:
                commands = {0xA, 0x12};
                break;
            case AlwaysOnUSBState::OnAlways:
                commands = {0xA, 0x13};
                break;
            default:
                return false;
        }
        
        for (uint32_t cmd : commands) {
            uint32_t dummyOutput = 0;
            if (!LenovoCommonUtils::EnergyDrvIoControl(IOCTL_ENERGY_SETTINGS, cmd, dummyOutput)) {
                return false;
            }
        }
        
        for (int retry = 0; retry < 10; ++retry) {
            auto currentState = GetAlwaysOnUSBState();
            if (currentState.has_value() && currentState.value() == state) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        return false;
    }
    
    inline bool IsAlwaysOnUSBSupported() {
        return GetAlwaysOnUSBState().has_value();
    }
} // namespace LenovoFeatures