#pragma once
#include <windows.h>
#include <cstdint>
#include "CommonUtils.hpp"

enum class WhiteKeyboardBacklightState {
    Off,
    Low,
    High
};

namespace LenovoWhiteKeyboardBacklightControl {
    inline bool IsSupported() {
        HANDLE hDriver = LenovoCommonUtils::GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        uint32_t inBuffer = 0x1; // Support check command
        uint32_t outBuffer = 0;
        
        BOOL success = DeviceIoControl(
            hDriver,
            0x83102144, // IOCTL_ENERGY_KEYBOARD
            &inBuffer, sizeof(inBuffer),
            &outBuffer, sizeof(outBuffer),
            &bytesReturned,
            nullptr
        );

        if (!success || bytesReturned != sizeof(uint32_t)) {
            return false;
        }

        // Right shift by 1 and check if equals 0x2
        return ((outBuffer >> 1) == 0x2);
    }

    inline bool GetState(WhiteKeyboardBacklightState& outState) {
        HANDLE hDriver = LenovoCommonUtils::GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        uint32_t inBuffer = 0x22; // Command to get state
        uint32_t outBuffer = 0;
        
        BOOL success = DeviceIoControl(
            hDriver,
            0x83102144, // IOCTL_ENERGY_KEYBOARD
            &inBuffer, sizeof(inBuffer),
            &outBuffer, sizeof(outBuffer),
            &bytesReturned,
            nullptr
        );

        if (!success || bytesReturned != sizeof(uint32_t)) {
            return false;
        }

        switch (outBuffer) {
            case 0x1: outState = WhiteKeyboardBacklightState::Off; return true;
            case 0x3: outState = WhiteKeyboardBacklightState::Low; return true;
            case 0x5: outState = WhiteKeyboardBacklightState::High; return true;
            default: return false;
        }
    }

    inline bool SetState(WhiteKeyboardBacklightState newState) {
        HANDLE hDriver = LenovoCommonUtils::GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        uint32_t command;
        switch (newState) {
            case WhiteKeyboardBacklightState::Off:  command = 0x00023; break;
            case WhiteKeyboardBacklightState::Low:  command = 0x10023; break;
            case WhiteKeyboardBacklightState::High: command = 0x20023; break;
            default: return false;
        }

        DWORD bytesReturned = 0;
        uint32_t dummyOutput = 0;
        
        BOOL success = DeviceIoControl(
            hDriver,
            0x83102144, // IOCTL_ENERGY_KEYBOARD
            &command, sizeof(command),
            &dummyOutput, sizeof(dummyOutput),
            &bytesReturned,
            nullptr
        );

        if (!success) {
            return false;
        }

        const int MAX_RETRIES = 10;
        const int DELAY_MS = 50;
        
        for (int i = 0; i < MAX_RETRIES; ++i) {
            WhiteKeyboardBacklightState currentState;
            if (GetState(currentState) && currentState == newState) {
                return true;
            }
            Sleep(DELAY_MS);
        }
        
        return false;
    }
} // namespace LenovoWhiteKeyboardBacklightControl