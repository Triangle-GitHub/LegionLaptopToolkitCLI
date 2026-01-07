#include<windows.h>
#include<iostream>
#include<algorithm>
#include<iomanip>
#include"LenovoBatteryControl.hpp"
#include"LenovoOverdriveControl.hpp"
#include"LenovoWhitekeyboardbacklightControl.hpp"
#include"LenovoPowerModeControl.hpp"

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
        << "  lltc set batterymode <Conservation|Normal|RapidCharge|1|2|3>\n"
        << "  lltc set overdrive <on|off|1|0>\n"
        << "  lltc set keyboardbacklight <off|low|high|0|1|2>\n"
        << "  lltc set powermode <Quiet|Balance|Performance|GodMode|1|2|3|254>\n";
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
            std::cerr << "Error: 'get' requires a property (batterymode/bm, overdrive/od, keyboardbacklight/kb, batteryinformation/bi, powermode/pm).\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        
        if (prop == "batteryinformation" || prop == "bi") {
            if (argc >= 4 && std::string(argv[3]) == "-dmon") {
                int refreshMs = 1000;
                if (argc >= 5) {
                    try {
                        refreshMs = std::stoi(argv[4]);
                        if (refreshMs < 10) {
                            std::cerr << "Error: refresh interval must be at least 10ms.\n";
                            return 1;
                        }
                    } catch (...) {
                        std::cerr << "Error: invalid refresh interval '" << argv[4] << "'. Must be a number >= 10.\n";
                        return 1;
                    }
                }
                GetFullBatteryInfoDmon(refreshMs);
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
        std::cerr << "Error: only 'powermode' (pm), 'batterymode' (bm), 'keyboardbacklight' (kb) and 'overdrive' (od) can be set.\n";
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

void GetFullBatteryInfoDmon(int ms) {
    GetFullBatteryInfo();
    std::cout << "======" << std::endl;
    
    std::cout<<std::setfill(' ');
    std::cout << std::right
              << std::setw(5) << "temp"
              << std::setw(5) << "pct"
              << std::setw(7) << "pwr"
              << std::setw(8) << "cap"
              << std::setw(6) << "cycle"
              << std::setw(6) << "islow"
              << std::endl;
    std::cout << std::right
              << std::setw(5) << "(C)"
              << std::setw(5) << "(%)"
              << std::setw(7) << "(W)"
              << std::setw(8) << "(Wh)"
              << std::setw(6) << "(s)"
              << std::setw(6) << "(0/1)"
              << std::endl;

    while (true) {
        BatteryInfoResult result = {0};
        if (!LenovoBatteryControl::GetBatteryInformation(result)) {
            Sleep(500);
            continue;
        }

        double pwrW = result.dischargeRate / 1000.0;
        char pwrBuf[12];
        snprintf(pwrBuf, sizeof(pwrBuf), "%+.2f", pwrW);
        std::string pwrStr = pwrBuf;

        double capWh = result.currentCapacity / 1000.0;
        char capBuf[12];
        snprintf(capBuf, sizeof(capBuf), "%.2f", capWh);
        std::string capStr = capBuf;

        std::cout << std::right;

        if (result.temperatureC >= 0) {
            std::cout << std::fixed << std::setprecision(1) << std::setw(5) << result.temperatureC;
        } else {
            std::cout << std::setw(5) << "N/A";
        }

        std::cout << std::setw(5) << static_cast<int>(result.batteryLifePercent)
                  << std::setw(7) << pwrStr
                  << std::setw(8) << capStr
                  << std::setw(6) << result.cycleCount
                  << std::setw(6) << (result.isLowBattery ? 1 : 0)
                  << std::endl;

        Sleep(ms);
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
            std::cerr << "Not supported.\n";
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