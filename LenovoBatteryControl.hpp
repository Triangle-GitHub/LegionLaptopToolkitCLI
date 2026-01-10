#pragma once

#include <windows.h>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <batclass.h>
#include <setupapi.h>
#include <devguid.h>
#include "CommonUtils.hpp"

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

// Lenovo-specific battery structure
#pragma pack(push, 2)
typedef struct _LENOVO_BATTERY_INFORMATION {
    uint8_t Reserved1[13];
    uint8_t Padding;
    uint16_t Temperature;
    uint16_t ManufactureDate;
    uint16_t FirstUseDate;
    uint8_t Reserved2[64];
} LENOVO_BATTERY_INFORMATION, *PLENOVO_BATTERY_INFORMATION;
#pragma pack(pop)

// Battery information result structure
struct BatteryInfoResult {
    bool isAcConnected;
    BYTE batteryLifePercent;
    DWORD batteryLifeTime;
    DWORD batteryFullLifeTime;
    LONG dischargeRate;
    DWORD currentCapacity;
    DWORD designedCapacity;
    DWORD fullChargedCapacity;
    DWORD cycleCount;
    bool isLowBattery;
    double temperatureC;
    SYSTEMTIME manufactureDate;
    SYSTEMTIME firstUseDate;
};

namespace LenovoBatteryControl {
    using LenovoCommonUtils::ReverseEndianness;
    using LenovoCommonUtils::ReverseEndianness16;
    using LenovoCommonUtils::GetNthBit;
    using LenovoCommonUtils::GetEnergyDriverHandle;
    using LenovoCommonUtils::GetBatteryHandle;
    using LenovoCommonUtils::GetBatteryTag;
    
    inline bool DecodeFATDate(uint16_t fatDate, SYSTEMTIME& outDate) {
        if (fatDate == 0) return false;
        
        uint16_t day = fatDate & 0x1F;
        uint16_t month = (fatDate >> 5) & 0x0F;
        uint16_t year = ((fatDate >> 9) & 0x7F) + 1980;

        if (year < 2018 || year > 2077 || month < 1 || month > 12 || day < 1 || day > 31)
            return false;

        outDate.wYear = year;
        outDate.wMonth = month;
        outDate.wDay = day;
        outDate.wHour = 0;
        outDate.wMinute = 0;
        outDate.wSecond = 0;
        outDate.wMilliseconds = 0;
        return true;
    }
    
    inline double DecodeTemperature(uint16_t rawTemp) {
        double tempC = (static_cast<double>(rawTemp) - 2731.6) / 10.0;
        return (tempC >= 0) ? tempC : -1.0;
    }

    inline bool GetLenovoBatteryInformation(uint32_t index, LENOVO_BATTERY_INFORMATION& outInfo) {
        HANDLE hDriver = GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }

        DWORD bytesReturned = 0;
        constexpr DWORD bufferSize = 256;
        BYTE buffer[bufferSize] = {0};

        BOOL success = DeviceIoControl(
            hDriver,
            0x83102138, // IOCTL_ENERGY_BATTERY_INFORMATION
            &index,
            sizeof(index),
            buffer,
            bufferSize,
            &bytesReturned,
            nullptr
        );
        if(!success) return false;

        memcpy(&outInfo, buffer, sizeof(LENOVO_BATTERY_INFORMATION));

        return true;
    }

    inline bool GetBatteryTemperatureC(double& outTempC) {
        LENOVO_BATTERY_INFORMATION info = {0};

        for (uint32_t i = 0; i < 3; i++) {
            if (GetLenovoBatteryInformation(i, info)) {
                if (info.Temperature != 0x0000 && info.Temperature != 0xFFFF) {
                    outTempC = DecodeTemperature(info.Temperature);
                    return (outTempC >= 0);
                }
            }
        }
        
        return false;
    }

    inline bool GetStandardBatteryInformation(ULONG batteryTag, BATTERY_INFORMATION& outInfo) {
        HANDLE hBattery = GetBatteryHandle();
        if (hBattery == INVALID_HANDLE_VALUE) {
            return false;
        }

        BATTERY_QUERY_INFORMATION query = { 
            batteryTag, 
            BatteryInformation, 
            0
        };

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            hBattery,
            IOCTL_BATTERY_QUERY_INFORMATION,
            &query,
            sizeof(query),
            &outInfo,
            sizeof(outInfo),
            &bytesReturned,
            nullptr
        );

        return success && (bytesReturned == sizeof(BATTERY_INFORMATION));
    }

    inline bool GetBatteryStatus(ULONG batteryTag, BATTERY_STATUS& outStatus) {
        HANDLE hBattery = GetBatteryHandle();
        if (hBattery == INVALID_HANDLE_VALUE) {
            return false;
        }

        BATTERY_WAIT_STATUS waitStatus = {0};
        waitStatus.BatteryTag = batteryTag;

        DWORD bytesReturned = 0;
        BOOL success = DeviceIoControl(
            hBattery,
            IOCTL_BATTERY_QUERY_STATUS,
            &waitStatus,
            sizeof(waitStatus),
            &outStatus,
            sizeof(outStatus),
            &bytesReturned,
            nullptr
        );
        return success && (bytesReturned == sizeof(BATTERY_STATUS));
    }

    inline bool GetBatteryInformation(BatteryInfoResult& outResult) {
        try {
            SYSTEM_POWER_STATUS sps = {0};
            if (!GetSystemPowerStatus(&sps)) {
                return false;
            }

            HANDLE hBattery = GetBatteryHandle();
            if (hBattery == INVALID_HANDLE_VALUE) {
                return false;
            }
            ULONG batteryTag = 0;
            if (!GetBatteryTag(hBattery, batteryTag)) {
                return false;
            }

            BATTERY_INFORMATION batInfo = {0};
            BATTERY_STATUS batStatus = {0};
            if (!GetStandardBatteryInformation(batteryTag, batInfo) ||
                !GetBatteryStatus(batteryTag, batStatus)) {
                return false;
            }

            LENOVO_BATTERY_INFORMATION lenovoInfo = {0};
            bool hasLenovoInfo = false;
            
            for (uint32_t i = 0; i < 3; i++) {
                if (GetLenovoBatteryInformation(i, lenovoInfo)) {
                    if (lenovoInfo.Temperature != 0x0000 && lenovoInfo.Temperature != 0xFFFF) {
                        hasLenovoInfo = true;
                        break;
                    }
                }
            }

            outResult.isAcConnected = (sps.ACLineStatus == 1);
            outResult.batteryLifePercent = sps.BatteryLifePercent;
            outResult.batteryLifeTime = static_cast<DWORD>(sps.BatteryLifeTime);
            outResult.batteryFullLifeTime = static_cast<DWORD>(sps.BatteryFullLifeTime);
            outResult.dischargeRate = batStatus.Rate;
            outResult.currentCapacity = batStatus.Capacity;
            outResult.designedCapacity = batInfo.DesignedCapacity;
            outResult.fullChargedCapacity = batInfo.FullChargedCapacity;
            outResult.cycleCount = batInfo.CycleCount;
            
            outResult.isLowBattery = (sps.ACLineStatus == 0) && 
                                    (batInfo.DefaultAlert2 >= batStatus.Capacity);

            if (hasLenovoInfo) {
                double tempC = DecodeTemperature(lenovoInfo.Temperature);
                outResult.temperatureC = (tempC >= 0) ? tempC : -1.0;
                
                SYSTEMTIME tempDate = {0};
                if (DecodeFATDate(lenovoInfo.ManufactureDate, tempDate)) {
                    outResult.manufactureDate = tempDate;
                } else {
                    memset(&outResult.manufactureDate, 0, sizeof(SYSTEMTIME));
                }
                
                if (DecodeFATDate(lenovoInfo.FirstUseDate, tempDate)) {
                    outResult.firstUseDate = tempDate;
                } else {
                    memset(&outResult.firstUseDate, 0, sizeof(SYSTEMTIME));
                }
            } else {
                outResult.temperatureC = -1.0;
                memset(&outResult.manufactureDate, 0, sizeof(SYSTEMTIME));
                memset(&outResult.firstUseDate, 0, sizeof(SYSTEMTIME));
            }
            
            return true;
        } catch (...) {
            memset(&outResult, 0, sizeof(BatteryInfoResult));
            outResult.temperatureC = -1.0;
            return false;
        }
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

    inline bool GetCurrentBatteryMode(BatteryState& outState) {
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
        } else {
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
                return false;
            }
        }

        return true;
    }

} // namespace LenovoBatteryControl