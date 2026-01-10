#include<windows.h>
#include<iostream>
#include<algorithm>
#include<iomanip>
#include<numeric>
#include"LenovoBatteryControl.hpp"
#include"LenovoOverdriveControl.hpp"
#include"LenovoWhitekeyboardbacklightControl.hpp"
#include"LenovoPowerModeControl.hpp"
#include"LenovoHybridmodeControl.hpp"

inline std::string toLower(const std::string& s);
bool TurnOffMonitor();
bool GetBatteryMode();
bool SetBatteryMode(int tar);
bool GetOverdrive();
bool SetOverdrive(int enable);
bool GetWhiteKeyboardBacklight();
bool SetWhiteKeyboardBacklight(int tar);
bool GetFullBatteryInfo();
void GetFullBatteryInfoDmon(int ms);
bool GetPowerMode();
bool SetPowerMode(int tar);
bool GetGPUMode();
bool SetGPUMode(int tar);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
        << "  lltc monitoroff | mo\n"
        << "  lltc get batterymode | bm\n"
        << "  lltc get overdrive | od\n"
        << "  lltc get keyboardbacklight | kb\n"
        << "  lltc get batteryinformation | bi\n"
        << "  lltc get batteryinformation -dmon\n"
        << "  lltc get powermode | pm\n"
        << "  lltc get gpumode | gm\n"
        << "  lltc set batterymode <Conservation|Normal|RapidCharge|1|2|3>\n"
        << "  lltc set overdrive <on|off|1|0>\n"
        << "  lltc set keyboardbacklight <off|low|high|0|1|2>\n"
        << "  lltc set powermode <Quiet|Balance|Performance|GodMode|1|2|3|254>\n"
        << "  lltc set gpumode <Hybrid|HybridIGPU|HybridAuto|dGPU|1|2|3|4>\n";
        return 1;
    }
    std::string cmd1 = toLower(argv[1]);
    // === lltc monitoroff / mo ===
    if (cmd1 == "monitoroff" || cmd1 == "mo") {
        TurnOffMonitor();
        return 0;
    }
    // === lltc get ... ===
    if (cmd1 == "get") {
        if (argc < 3) {
            std::cerr << "Error: 'get' requires a property (batterymode/bm, overdrive/od, keyboardbacklight/kb, batteryinformation/bi, powermode/pm, gpumode/gm).\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        
        if (prop == "batteryinformation" || prop == "bi") {
            if (argc >= 4 && std::string(argv[3]) == "-dmon") {
                int refreshS = 0;
                if (argc >= 5) {
                    try {
                        refreshS = std::stoi(argv[4]);
                        if (refreshS < 1) {
                            std::cerr << "Error: refresh interval must be at least 1s.\n";
                            return 1;
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid refresh interval '" << argv[4] << "'. Must be a number >= 1.\n";
                        return 1;
                    }
                }
                GetFullBatteryInfoDmon(refreshS);
                return 0;
            } else {
                return GetFullBatteryInfo() ? 0 : 1;
            }
        } else if (prop == "batterymode" || prop == "bm") {
            return GetBatteryMode() ? 0 : 1;
        } else if (prop == "overdrive" || prop == "od") {
            return GetOverdrive() ? 0 : 1;
        } else if (prop == "keyboardbacklight" || prop == "kb") {
            return GetWhiteKeyboardBacklight() ? 0 : 1;
        } else if (prop == "powermode" || prop == "pm") {
            return GetPowerMode() ? 0 : 1;
        } else if (prop == "gpumode" || prop == "gm") {
            return GetGPUMode() ? 0 : 1;
        } else {
            std::cerr << "Error: unknown property '" << argv[2] << "'.\n";
            return 1;
        }
    }
    // === lltc set ... ===
    if (cmd1 == "set") {
        if (argc < 3) {
            std::cerr << "Error: 'set' requires a property.\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        
        // --- GPU Mode ---
        if (prop == "gpumode" || prop == "gm") {
            if (argc < 4) {
                std::cerr << "Error: missing GPU mode value.\n";
                return 1;
            }
            std::string value = toLower(argv[3]);
            int modeInt = -1;
            
            try {
                size_t pos;
                int num = std::stoi(value, &pos);
                if (pos == value.size() && num >= 1 && num <= 4) {
                    modeInt = num;
                }
            } catch (...) {
                // not a number
            }
            
            if (modeInt == -1) {
                if (value == "hybrid") {
                    modeInt = 1;
                } else if (value == "hybridigpu") {
                    modeInt = 2;
                } else if (value == "hybridauto") {
                    modeInt = 3;
                } else if (value == "dgpu") {
                    modeInt = 4;
                }
            }
            
            if (modeInt == -1) {
                std::cerr << "Error: invalid GPU mode '" << argv[3]
                    << "'. Use Hybrid/HybridIGPU/HybridAuto/dGPU or 1/2/3/4.\n";
                return 1;
            }
            return SetGPUMode(modeInt) ? 0 : 1;
        }
        
        // --- Power Mode ---
        if (prop == "powermode" || prop == "pm") {
            if (argc < 4) {
                std::cerr << "Error: missing power mode value.\n";
                return 1;
            }
            std::string value = toLower(argv[3]);
            int modeInt = -1;
            
            try {
                size_t pos;
                int num = std::stoi(value, &pos);
                if (pos == value.size()) {
                    if (num == 1 || num == 2 || num == 3 || num == 254) {
                        modeInt = num;
                    }
                }
            } catch (...) {
                // not a number
            }
            
            if (modeInt == -1) {
                if (value == "quiet") {
                    modeInt = 1;
                } else if (value == "balance") {
                    modeInt = 2;
                } else if (value == "performance") {
                    modeInt = 3;
                } else if (value == "godmode") {
                    modeInt = 254;
                }
            }
            
            if (modeInt == -1) {
                std::cerr << "Error: invalid power mode '" << argv[3]
                    << "'. Use Quiet/Balance/Performance/GodMode or 1/2/3/254.\n";
                return 1;
            }
            return SetPowerMode(modeInt) ? 0 : 1;
        }
        
        // --- Battery Mode ---
        if (prop == "batterymode" || prop == "bm") {
            if (argc < 4) {
                std::cerr << "Error: missing battery mode value.\n";
                return 1;
            }
            std::string value = toLower(argv[3]);
            int modeInt = -1;
            try {
                size_t pos;
                int num = std::stoi(value, &pos);
                if (pos == value.size() && num >= 1 && num <= 3) {
                    modeInt = num;
                }
            } catch (...) {
                // not a number
            }
            if (modeInt == -1) {
                if (value == "conservation") {
                    modeInt = 1;
                } else if (value == "normal") {
                    modeInt = 2;
                } else if (value == "rapidcharge") {
                    modeInt = 3;
                } else {
                    std::cerr << "Error: invalid battery mode '" << argv[3]
                        << "'. Use Conservation/Normal/RapidCharge or 1/2/3.\n";
                    return 1;
                }
            }
            return SetBatteryMode(modeInt) ? 0 : 1;
        }
        // --- Keyboard Backlight ---
        if (prop == "keyboardbacklight" || prop == "kb") {
            if (argc < 4) {
                std::cerr << "Error: missing keyboard backlight value.\n";
                return 1;
            }
            std::string value = toLower(argv[3]);
            int levelInt = -1;
            try {
                size_t pos;
                int num = std::stoi(value, &pos);
                if (pos == value.size() && num >= 0 && num <= 2) {
                    levelInt = num;
                }
            } catch (...) {
                // not a number
            }
            if (levelInt == -1) {
                if (value == "off") {
                    levelInt = 0;
                } else if (value == "low") {
                    levelInt = 1;
                } else if (value == "high") {
                    levelInt = 2;
                } else {
                    std::cerr << "Error: invalid keyboard backlight level '" << argv[3]
                        << "'. Use off/low/high or 0/1/2.\n";
                    return 1;
                }
            }
            return SetWhiteKeyboardBacklight(levelInt) ? 0 : 1;
        }
        // --- Overdrive (od / overdrive) ---
        if (prop == "overdrive" || prop == "od") {
            if (argc < 4) {
                std::cerr << "Error: missing overdrive value (on/off/1/0).\n";
                return 1;
            }
            std::string value = toLower(argv[3]);
            int enable = -1;
            if (value == "on" || value == "1") {
                enable = 1;
            } else if (value == "off" || value == "0") {
                enable = 0;
            } else {
                std::cerr << "Error: invalid overdrive value '" << argv[3]
                    << "'. Use on/off or 1/0.\n";
                return 1;
            }
            return SetOverdrive(enable) ? 0 : 1;
        }
        // --- Unknown property ---
        std::cerr << "Error: only 'powermode' (pm), 'batterymode' (bm), 'keyboardbacklight' (kb), 'overdrive' (od), and 'gpumode' (gm) can be set.\n";
        return 1;
    }
    std::cerr << "Error: unknown command '" << argv[1] << "'.\n";
    return 1;
}

inline std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

bool TurnOffMonitor(){
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
    return true;
}

bool GetBatteryMode(){
    BatteryState currentMode;
    if (LenovoBatteryControl::GetCurrentBatteryMode(currentMode)) {
        switch (currentMode) {
            case BatteryState::Conservation:
                std::cout << "Conservation\n";
                break;
            case BatteryState::Normal:
                std::cout << "Normal\n";
                break;
            case BatteryState::RapidCharge:
                std::cout << "RapidCharge\n";
                break;
        }
    } else return false;
    return true;
}

bool SetBatteryMode(int tar){
    BatteryState state;
    switch(tar){
        case 1:
            state=BatteryState::Conservation;
            break;
        case 2:
            state=BatteryState::Normal;
            break;
        case 3:
            state=BatteryState::RapidCharge;
            break;
        default:
            std::cerr<<"Invalid battery mode.\n";
            return false;
    }
    bool success = LenovoBatteryControl::SetBatteryMode(state);
    if (success) {
        switch (state) {
            case BatteryState::Conservation:
                std::cout << "Conservation\n";
                break;
            case BatteryState::Normal:
                std::cout << "Normal\n";
                break;
            case BatteryState::RapidCharge:
                std::cout << "RapidCharge\n";
                break;
        }
    } else {
        std::cerr << "Switch failed.\n";
        return false;
    }
    return true;
}

bool SetOverdrive(int enable) {
    if (enable != 0 && enable != 1) {
        std::cerr << "Not supported or permission denined.\n";
        return false;
    }

    OverDriveController controller;
    if (!controller.IsOverDriveSupported()) {
        std::cerr << "Not supported or permission denined.\n";
        return false;
    }

    HRESULT hr = controller.SetOverDriveStatus(enable == 1);
    if (SUCCEEDED(hr)) {
        if (enable == 1) {
            std::cout << "Overdrive enabled\n";
        } else {
            std::cout << "Overdrive disabled\n";
        }
        return true;
    } else {
        std::cerr << "Failed to set Overdrive status.\n";
        return false;
    }
}

bool GetOverdrive() {
    OverDriveController controller;
    if (!controller.IsOverDriveSupported()) {
        std::cerr << "Not supported or permission denined.\n";
        return false;
    }

    int status = controller.GetOverDriveStatus();
    if (status == 1) {
        std::cout << "Overdrive ON\n";
        return true;
    } else if (status == 0) {
        std::cout << "Overdrive OFF\n";
        return true;
    } else {
        std::cerr << "Failed to get Overdrive status.\n";
        return false;
    }
}

bool GetWhiteKeyboardBacklight() {
    WhiteKeyboardBacklightState currentState;
    if (LenovoWhiteKeyboardBacklightControl::GetState(currentState)) {
        switch (currentState) {
            case WhiteKeyboardBacklightState::Off:
                std::cout << "Off\n";
                break;
            case WhiteKeyboardBacklightState::Low:
                std::cout << "Low\n";
                break;
            case WhiteKeyboardBacklightState::High:
                std::cout << "High\n";
                break;
        }
    } else {
        std::cerr << "Failed to get keyboard backlight state.\n";
        return false;
    }
    return true;
}

bool SetWhiteKeyboardBacklight(int tar) {
    WhiteKeyboardBacklightState state;
    switch(tar) {
        case 0:
            state = WhiteKeyboardBacklightState::Off;
            break;
        case 1:
            state = WhiteKeyboardBacklightState::Low;
            break;
        case 2:
            state = WhiteKeyboardBacklightState::High;
            break;
        default:
            std::cerr << "Invalid backlight level. Use 0 for Off, 1 for Low, or 2 for High.\n";
            return false;
    }
    
    bool success = LenovoWhiteKeyboardBacklightControl::SetState(state);
    if (success) {
        switch (state) {
            case WhiteKeyboardBacklightState::Off:
                std::cout << "Off\n";
                break;
            case WhiteKeyboardBacklightState::Low:
                std::cout << "Low\n";
                break;
            case WhiteKeyboardBacklightState::High:
                std::cout << "High\n";
                break;
        }
    } else {
        std::cerr << "Failed to set keyboard backlight state.\n";
        return false;
    }
    return true;
}

bool GetFullBatteryInfo() {
    BatteryInfoResult result = {0};

    if (!LenovoBatteryControl::GetBatteryInformation(result)) {
        std::cerr << "Failed to get battery information" << std::endl;
        return false;
    }
    std::cout << "AC Connected: " << (result.isAcConnected ? "Yes" : "No") << std::endl;
    std::cout << "Battery Life: " << static_cast<int>(result.batteryLifePercent) << "%" << std::endl;
    
    if (result.batteryLifeTime != 0xFFFFFFFF) {
        std::cout << "Estimated Remaining Time: " 
                  << result.batteryLifeTime / 3600 << "h " 
                  << (result.batteryLifeTime % 3600) / 60 << "m" << std::endl;
    }
    
    std::cout << "Discharge Rate: " << result.dischargeRate << " mW" << std::endl;
    std::cout << "Current Capacity: " << result.currentCapacity << " mWh" << std::endl;
    std::cout << "Designed Capacity: " << result.designedCapacity << " mWh" << std::endl;
    std::cout << "Full Charged Capacity: " << result.fullChargedCapacity << " mWh" << std::endl;
    std::cout << "Cycle Count: " << result.cycleCount << std::endl;
    std::cout << "Low Battery Alert: " << (result.isLowBattery ? "Yes" : "No") << std::endl;

    std::cout << "======" << std::endl;
    
    if (result.temperatureC >= 0) {
        std::cout << "Battery Temperature: " << std::fixed << std::setprecision(1) 
                  << result.temperatureC << " C" << std::endl;
    } else {
        std::cout << "Battery Temperature: Not available" << std::endl;
    }

    if (result.manufactureDate.wYear != 0) {
        std::cout << "Manufacture Date: " 
                  << result.manufactureDate.wYear << "-" 
                  << std::setw(2) << std::setfill('0') << result.manufactureDate.wMonth << "-" 
                  << std::setw(2) << std::setfill('0') << result.manufactureDate.wDay << std::endl;
    } else {
        std::cout << "Manufacture Date: Not available" << std::endl;
    }

    if (result.firstUseDate.wYear != 0) {
        std::cout << "First Use Date: " 
                  << result.firstUseDate.wYear << "-" 
                  << std::setw(2) << std::setfill('0') << result.firstUseDate.wMonth << "-" 
                  << std::setw(2) << std::setfill('0') << result.firstUseDate.wDay << std::endl;
    } else {
        std::cout << "First Use Date: Not available" << std::endl;
    }
    return true;
}

void GetFullBatteryInfoDmon(int seconds) {
    GetFullBatteryInfo();
    std::cout << "======" << std::endl;
    
    const int time_col_width = 20;
    const int non_time_col_width = 8; // Unified width for all non-time columns
    
    std::cout << std::setfill(' ') << std::right;
    
    bool dft = false;
    // Determine header format based on interval
    if (seconds == 0) {
        seconds = 1;
        dft = true;
        std::cout 
            << std::setw(time_col_width) << " "
            << std::setw(non_time_col_width) << "AC"
            << std::setw(non_time_col_width) << "temp"
            << std::setw(non_time_col_width) << "pct"
            << std::setw(non_time_col_width) << "pwr"
            << std::setw(non_time_col_width) << "cap"
            << std::setw(non_time_col_width) << "cycle"
            << std::setw(non_time_col_width) << "low"
            << std::endl;
        std::cout 
            << std::setw(time_col_width) << " "
            << std::setw(non_time_col_width) << " "
            << std::setw(non_time_col_width) << "(C)"
            << std::setw(non_time_col_width) << "(%)"
            << std::setw(non_time_col_width) << "(W)"
            << std::setw(non_time_col_width) << "(Wh)"
            << std::setw(non_time_col_width) << "(s)"
            << std::setw(non_time_col_width) << "(Y/N)"
            << std::endl;
    } else {
        std::cout 
            << std::setw(time_col_width) << " "
            << std::setw(non_time_col_width) << "AC"
            << std::setw(non_time_col_width) << "temp"
            << std::setw(non_time_col_width) << "tempAvg"
            << std::setw(non_time_col_width) << "percent"
            << std::setw(non_time_col_width) << "power"
            << std::setw(non_time_col_width) << "pwrAvg"
            << std::setw(non_time_col_width) << "cap"
            << std::setw(non_time_col_width) << "cycle"
            << std::setw(non_time_col_width) << "lowCap"
            << std::endl;
        std::cout 
            << std::setw(time_col_width) << " "
            << std::setw(non_time_col_width) << " "
            << std::setw(non_time_col_width) << "(C)"
            << std::setw(non_time_col_width) << "(C)"
            << std::setw(non_time_col_width) << "(%)"
            << std::setw(non_time_col_width) << "(W)"
            << std::setw(non_time_col_width) << "(W)"
            << std::setw(non_time_col_width) << "(Wh)"
            << std::setw(non_time_col_width) << "(s)"
            << std::setw(non_time_col_width) << ""
            << std::endl;
    }

    int count = 0;
    std::vector<double> tempSamples;
    std::vector<double> powerSamples;

    while (true) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeBuf[21];
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02d %02d:%02d:%02d", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        std::string timeStr = timeBuf;

        BatteryInfoResult result = {0};
        if (!LenovoBatteryControl::GetBatteryInformation(result)) {
            Sleep(1000);
            continue;
        }

        double currentTemp = result.temperatureC;
        double currentPower = result.dischargeRate / 1000.0;

        if (!dft) {
            if (currentTemp >= 0) {
                tempSamples.push_back(currentTemp);
            }
            powerSamples.push_back(currentPower);
        }

        if (seconds == 1 || count == seconds - 1) {
            double avgTemp = -1.0; // Invalid marker
            double avgPower = 0.0;
            
            if (!dft && !tempSamples.empty()) {
                avgTemp = std::accumulate(tempSamples.begin(), tempSamples.end(), 0.0) / tempSamples.size();
            }
            if (!dft && !powerSamples.empty()) {
                avgPower = std::accumulate(powerSamples.begin(), powerSamples.end(), 0.0) / powerSamples.size();
            }

            std::cout << std::setw(time_col_width) << timeStr;
            
            std::cout << std::setw(non_time_col_width) << (result.isAcConnected ? "Y" : "N");
            
            if (currentTemp >= 0) {
                char tempBuf[12];
                snprintf(tempBuf, sizeof(tempBuf), "%.1f", currentTemp);
                std::cout << std::setw(non_time_col_width) << tempBuf;
            } else {
                std::cout << std::setw(non_time_col_width) << "N/A";
            }
            
            if (!dft) {
                char avgTempBuf[12];
                if (avgTemp >= 0) {
                    snprintf(avgTempBuf, sizeof(avgTempBuf), "%.3f", avgTemp);
                } else {
                    snprintf(avgTempBuf, sizeof(avgTempBuf), "N/A");
                }
                std::cout << std::setw(non_time_col_width) << avgTempBuf;
            }
            
            char pctBuf[4];
            snprintf(pctBuf, sizeof(pctBuf), "%d", static_cast<int>(result.batteryLifePercent));
            std::cout << std::setw(non_time_col_width) << pctBuf;
            
            char pwrBuf[12];
            snprintf(pwrBuf, sizeof(pwrBuf), "%+.2f", currentPower);
            std::cout << std::setw(non_time_col_width) << pwrBuf;
            
            if (!dft) {
                char avgPwrBuf[12];
                snprintf(avgPwrBuf, sizeof(avgPwrBuf), "%+.3f", avgPower);
                std::cout << std::setw(non_time_col_width) << avgPwrBuf;
            }
            
            double capWh = result.currentCapacity / 1000.0;
            char capBuf[12];
            snprintf(capBuf, sizeof(capBuf), "%.2f", capWh);
            std::cout << std::setw(non_time_col_width) << capBuf;
            
            char cycleBuf[12];
            snprintf(cycleBuf, sizeof(cycleBuf), "%lu", result.cycleCount);
            std::cout << std::setw(non_time_col_width) << cycleBuf;
            
            std::cout << std::setw(non_time_col_width) << (result.isLowBattery ? "Y" : "N")
                      << std::endl;
            
            if (!dft) {
                tempSamples.clear();
                powerSamples.clear();
            }
            count = 0;
        } else {
            count++;
        }

        Sleep(1000); // Sleep for 1 second between readings
    }
}

bool GetPowerMode() {
    PowerMode currentMode;
    if (LegionPowerMode::GetPowerMode(currentMode)) {
        switch (currentMode) {
            case PowerMode::Quiet:
                std::cout << "Quiet\n";
                break;
            case PowerMode::Balance:
                std::cout << "Balance\n";
                break;
            case PowerMode::Performance:
                std::cout << "Performance\n";
                break;
            case PowerMode::GodMode:
                std::cout << "GodMode\n";
                break;
        }
        return true;
    } else {
        std::cerr << "Failed to get current power mode.\n";
        return false;
    }
}

bool SetPowerMode(int tar) {
    PowerMode mode;
    switch(tar) {
        case 1:
            mode = PowerMode::Quiet;
            break;
        case 2:
            mode = PowerMode::Balance;
            break;
        case 3:
            mode = PowerMode::Performance;
            break;
        case 254:
            mode = PowerMode::GodMode;
            std::cerr << "GodMode not supported.\n";
            return false;
        default:
            std::cerr << "Invalid power mode.\n";
            return false;
    }
    
    if (LegionPowerMode::SetPowerMode(mode)) {
        switch (mode) {
            case PowerMode::Quiet:
                std::cout << "Quiet\n";
                break;
            case PowerMode::Balance:
                std::cout << "Balance\n";
                break;
            case PowerMode::Performance:
                std::cout << "Performance\n";
                break;
            case PowerMode::GodMode:
                std::cout << "GodMode\n";
                break;
        }
        return true;
    } else {
        std::cerr << "Switch failed.\n";
        return false;
    }
}

bool GetGPUMode() {
    HybridModeState currentState;
    HybridModeController controller;
    
    auto future = controller.GetHybridModeAsync();
    auto [result, mode] = future.get();
    
    if (result == OperationResult::Success) {
        currentState = mode;
        
        switch (currentState) {
            case HybridModeState::On:
                std::cout << "Hybrid\n";
                break;
            case HybridModeState::OnIGPUOnly:
                std::cout << "Hybrid-iGPU\n";
                break;
            case HybridModeState::OnAuto:
                std::cout << "Hybrid-Auto\n";
                break;
            case HybridModeState::Off:
                std::cout << "dGPU\n";
                break;
        }
        return true;
    } else {
        std::cerr << "Failed to get current GPU mode.\n";
        return false;
    }
}

bool SetGPUMode(int tar) {
    HybridModeController controller;
    HybridModeState targetMode;
    
    switch(tar) {
        case 1: targetMode = HybridModeState::On; break;
        case 2: targetMode = HybridModeState::OnIGPUOnly; break;
        case 3: targetMode = HybridModeState::OnAuto; break;
        case 4: targetMode = HybridModeState::Off; break;
        default:
            std::cerr << "Invalid GPU mode.\n";
            return false;
    }
    
    auto future_get = controller.GetHybridModeAsync();
    auto [result_get, currentState] = future_get.get();
    if (result_get != OperationResult::Success) {
        std::cerr << "Failed to get current GPU mode.\n";
        return false;
    }

    if (currentState == targetMode) {
        std::cout << "Already in mode: ";
        switch (targetMode) {
            case HybridModeState::On: std::cout << "Hybrid\n"; break;
            case HybridModeState::OnIGPUOnly: std::cout << "Hybrid-iGPU\n"; break;
            case HybridModeState::OnAuto: std::cout << "Hybrid-Auto\n"; break;
            case HybridModeState::Off: std::cout << "dGPU\n"; break;
        }
        return true;
    }

    auto future_set = controller.SetHybridModeAsync(targetMode);
    OperationResult result_set = future_set.get();
    if (result_set != OperationResult::Success) {
        std::cerr << "Switch failed.\n";
        return false;
    }

    const bool switchingToDGpu = (targetMode == HybridModeState::Off);
    const bool switchingFromDGpu = (currentState == HybridModeState::Off);
    const bool requiresReboot = switchingToDGpu || switchingFromDGpu;

    switch (targetMode) {
        case HybridModeState::On: std::cout << "Hybrid\n"; break;
        case HybridModeState::OnIGPUOnly: std::cout << "Hybrid-iGPU\n"; break;
        case HybridModeState::OnAuto: std::cout << "Hybrid-Auto\n"; break;
        case HybridModeState::Off: std::cout << "dGPU\n"; break;
    }

    if (requiresReboot) {
        std::cout << "\n*** SYSTEM RESTART REQUIRED ***\n";
        std::cout << "Press ANY KEY to restart immediately...\n";
        std::cin.get();
        
        std::system("shutdown /r /t 0");
        return true;
    }
    return true;
}