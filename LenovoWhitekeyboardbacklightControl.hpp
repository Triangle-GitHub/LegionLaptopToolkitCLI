#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>

enum class WhiteKeyboardBacklightState {
    Off,
    Low,
    High
};

namespace LenovoWhiteKeyboardBacklightControl {
    // Built-in tools
    inline uint32_t ReverseEndianness(uint32_t value) {
        return ((value & 0x000000FFU) << 24) |
               ((value & 0x0000FF00U) << 8)  |
               ((value & 0x00FF0000U) >> 8)  |
               ((value & 0xFF000000U) >> 24);
    }

    // Get EnergyDrv driver handle
    inline HANDLE GetEnergyDriverHandle() {
        static HANDLE hDriver = INVALID_HANDLE_VALUE;
        static bool initialized = false;
        static CRITICAL_SECTION cs;
        static bool csInitialized = false;
        
        if (!csInitialized) {
            InitializeCriticalSection(&cs);
            csInitialized = true;
        }
        
        EnterCriticalSection(&cs);
        if (!initialized) {
            hDriver = CreateFileW(
                L"\\\\.\\EnergyDrv",
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            initialized = true;
        }
        LeaveCriticalSection(&cs);
        
        return hDriver;
    }

    // Check if device supports white keyboard backlight control
    inline bool IsSupported() {
        HANDLE hDriver = GetEnergyDriverHandle();
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

    // Get current white keyboard backlight state
    inline bool GetState(WhiteKeyboardBacklightState& outState) {
        HANDLE hDriver = GetEnergyDriverHandle();
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

        // Map driver return value to state
        switch (outBuffer) {
            case 0x1: outState = WhiteKeyboardBacklightState::Off; return true;
            case 0x3: outState = WhiteKeyboardBacklightState::Low; return true;
            case 0x5: outState = WhiteKeyboardBacklightState::High; return true;
            default: return false;
        }
    }

    // Set white keyboard backlight state
    inline bool SetState(WhiteKeyboardBacklightState newState) {
        HANDLE hDriver = GetEnergyDriverHandle();
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

        // Verify state was set correctly
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