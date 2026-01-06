#pragma once

#include <windows.h>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <objbase.h>
#include <Wbemidl.h>
#include <comdef.h>

enum class BatteryState {
    Conservation,
    Normal,
    RapidCharge
};
enum class ChargingState {
    Connected,
    ConnectedLowWattage,
    Disconnected
};

namespace LenovoBatteryControl {
    //Built-in tools
    inline uint32_t ReverseEndianness(uint32_t value) {
        return ((value & 0x000000FFU) << 24) |
               ((value & 0x0000FF00U) << 8)  |
               ((value & 0x00FF0000U) >> 8)  |
               ((value & 0xFF000000U) >> 24);
    }
    inline bool GetNthBit(uint32_t value, int n) {
        return (value & (1U << n)) != 0;
    }

    //Get EnergyDrv driver handle
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

            if (hDriver == INVALID_HANDLE_VALUE) {
                DWORD err = GetLastError(); //debug
            }
            initialized = true;
        }
        LeaveCriticalSection(&cs);

        return hDriver;
    }

    inline bool GetChargingState(ChargingState& outState) {
        SYSTEM_POWER_STATUS sps = {0};
        if (!GetSystemPowerStatus(&sps)) {
            return false;
        }
        if (sps.ACLineStatus == 1) outState = ChargingState::Connected;
        else outState = ChargingState::Disconnected;
        return true;
    }

    inline bool GetCurrentBatteryMode(BatteryState& outState) { //Output via outState
        HANDLE hDriver = GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        uint32_t inBuffer = 0xFFFFFFFF;
        uint32_t outBuffer = 0;

        BOOL success = DeviceIoControl(
            hDriver,
            0x831020F8, //IOCTL_ENERGY_BATTERY_CHARGE_MODE
            &inBuffer, sizeof(inBuffer),
            &outBuffer, sizeof(outBuffer),
            &bytesReturned,
            nullptr
        );

        if (!success || bytesReturned != sizeof(uint32_t)) {
            return false;
        }

        uint32_t state = ReverseEndianness(outBuffer);

        if (GetNthBit(state, 17)) { //Charging
            outState = GetNthBit(state, 26) ? BatteryState::RapidCharge : BatteryState::Normal;
        } else if (GetNthBit(state, 29)) { //Conservation
            outState = BatteryState::Conservation;
        } else { //Unknown status
            return false;
        }

        return true;
    }

    inline bool SetBatteryMode(BatteryState newState) {
        HANDLE hDriver = GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        BatteryState currentState = BatteryState::Normal;
        if(!GetCurrentBatteryMode(currentState)) return false;

        std::vector<uint32_t> commands;

        switch (newState) {
        case BatteryState::Conservation:
            if (currentState == BatteryState::RapidCharge) {
                commands = { 0x8, 0x3 };
            } else {
                commands = { 0x3 };
            }
            break;

        case BatteryState::Normal:
            if (currentState == BatteryState::Conservation) {
                commands = { 0x5 };
            } else {
                commands = { 0x8 };
            }
            break;

        case BatteryState::RapidCharge:
            if (currentState == BatteryState::Conservation) {
                commands = { 0x5, 0x7 };
            } else {
                commands = { 0x7 };
            }
            break;

        default:
            return false;
        }

        DWORD bytesReturned;
        uint32_t dummyOutput = 0;
        for (uint32_t cmd : commands) {
            BOOL success = DeviceIoControl(
                hDriver,
                0x831020F8, // IOCTL_ENERGY_BATTERY_CHARGE_MODE
                &cmd, sizeof(cmd),
                &dummyOutput, sizeof(dummyOutput),
                &bytesReturned,
                nullptr
            );
            if (!success) {
                DWORD err = GetLastError(); //debug
                return false;
            }
        }

        return true;
    }

} // namespace LenovoBatteryControl
