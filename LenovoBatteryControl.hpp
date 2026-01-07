#pragma once

#include <windows.h>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <objbase.h>
#include <Wbemidl.h>
#include <comdef.h>
#include <batclass.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

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
#pragma pack(push, 2) // 关键：使用2字节对齐（匹配C# ushort对齐要求）
typedef struct _LENOVO_BATTERY_INFORMATION {
    uint8_t Reserved1[13]; // 对应 C# 的 Bytes1 (13字节)
    uint8_t Padding;       // 手动添加1字节填充（使Temperature对齐到2字节边界）
    uint16_t Temperature;  // 偏移14-15 (0.1 Kelvin units)
    uint16_t ManufactureDate; // 偏移16-17 (FAT16 date format)
    uint16_t FirstUseDate;   // 偏移18-19 (FAT16 date format)
    uint8_t Reserved2[64];   // 对应 C# 的 Bytes2 (64字节)
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
    inline uint16_t ReverseEndianness16(uint16_t value) {
        return ((value & 0x00FFU) << 8) |
               ((value & 0xFF00U) >> 8);
    }

    inline bool DecodeFATDate(uint16_t fatDate, SYSTEMTIME& outDate) {
        if (fatDate == 0) return false;
        
        uint16_t day = fatDate & 0x1F;
        uint16_t month = (fatDate >> 5) & 0x0F;
        uint16_t year = ((fatDate >> 9) & 0x7F) + 1980;

        // Basic validation (reasonable year range)
        if (year < 2018 || year > 2030 || month < 1 || month > 12 || day < 1 || day > 31)
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
        // Convert from 0.1 Kelvin to Celsius
        double tempC = (static_cast<double>(rawTemp) - 2731.6) / 10.0;
        return (tempC >= 0) ? tempC : -1.0; // Return -1.0 for invalid temperatures
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

    // Get standard battery handle (for non-Lenovo specific info)
    inline HANDLE GetBatteryHandle() {
        static HANDLE hBattery = INVALID_HANDLE_VALUE;
        static bool initialized = false;
        static CRITICAL_SECTION cs;
        static bool csInitialized = false;

        if (!csInitialized) {
            InitializeCriticalSection(&cs);
            csInitialized = true;
        }

        EnterCriticalSection(&cs);
        if (!initialized) {
            // Use SetupAPI to find battery device (like C# code)
            GUID guidBattery = {0x72631e54, 0x78A4, 0x11d0, {0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a}}; // GUID_DEVCLASS_BATTERY
            
            HDEVINFO hDevInfo = SetupDiGetClassDevsW(
                &guidBattery,
                nullptr,
                nullptr,
                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
            );

            if (hDevInfo != INVALID_HANDLE_VALUE) {
                SP_DEVICE_INTERFACE_DATA devInterfaceData = {0};
                devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

                if (SetupDiEnumDeviceInterfaces(
                    hDevInfo,
                    nullptr,
                    &guidBattery,
                    0, // First device
                    &devInterfaceData
                )) {
                    DWORD requiredSize = 0;
                    SetupDiGetDeviceInterfaceDetailW(
                        hDevInfo,
                        &devInterfaceData,
                        nullptr,
                        0,
                        &requiredSize,
                        nullptr
                    );

                    if (requiredSize > 0) {
                        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData = 
                            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);
                        if (detailData) {
                            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
                            
                            if (SetupDiGetDeviceInterfaceDetailW(
                                hDevInfo,
                                &devInterfaceData,
                                detailData,
                                requiredSize,
                                nullptr,
                                nullptr
                            )) {
                                // Open the actual device path (dynamic!)
                                hBattery = CreateFileW(
                                    detailData->DevicePath,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr
                                );
                            }
                            free(detailData);
                        }
                    }
                }
                SetupDiDestroyDeviceInfoList(hDevInfo);
            }
            initialized = true;
        }
        LeaveCriticalSection(&cs);
        return hBattery;
    }

    // Get battery tag (required for standard battery IOCTLs)
    inline bool GetBatteryTag(HANDLE hBattery, ULONG& outTag) {
        DWORD dwWait = 0;
        DWORD dwBytesReturned = 0;

        BOOL success = DeviceIoControl(
            hBattery,
            2703424U, // IOCTL_BATTERY_QUERY_TAG
            &dwWait,
            sizeof(dwWait),
            &outTag,
            sizeof(outTag),
            &dwBytesReturned,
            nullptr
        );

        return success && (dwBytesReturned == sizeof(outTag)) && (outTag != 0);
    }
    
    // Get Lenovo-specific battery information
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

        memcpy(&outInfo, buffer, sizeof(LENOVO_BATTERY_INFORMATION));

        return true;
    }

    // Get temperature in Celsius
    inline bool GetBatteryTemperatureC(double& outTempC) {
        LENOVO_BATTERY_INFORMATION info = {0};

        // Try indexes 0, 1, 2 to find valid battery data
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

    // Get standard battery information
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

    // Get standard battery status
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

    // Main battery information function
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
                // Set invalid values for missing Lenovo data
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
