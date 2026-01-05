#include<windows.h>
#include<iostream>
#include<algorithm>
#include"LenovoBatteryControl.hpp"

inline std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

bool TurnOffMonitor();
bool GetBatteryMode();
bool SetBatteryMode(int tar);
bool GetBatteryIsCharging(void);
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  lltc monitoroff | mo\n"
                  << "  lltc get ischarging | ic\n"
                  << "  lltc get batterymode | bm\n"
                  << "  lltc set batterymode <Conservation|Normal|RapidCharge|1|2|3>\n";
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
            std::cerr << "Error: 'get' requires a property (ischarging/ic, batterymode/bm).\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        if (prop == "ischarging" || prop == "ic") {
            return GetBatteryIsCharging() ? 0 : 1;
        } else if (prop == "batterymode" || prop == "bm") {
            return GetBatteryMode() ? 0 : 1;
        } else {
            std::cerr << "Error: unknown property '" << argv[2] << "'.\n";
            return 1;
        }
    }
    // === lltc set batterymode ... ===
    if (cmd1 == "set") {
        if (argc < 3) {
            std::cerr << "Error: 'set' requires a property.\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        if (prop != "batterymode" && prop != "bm") {
            std::cerr << "Error: only 'batterymode' (or 'bm') can be set.\n";
            return 1;
        }
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
    std::cerr << "Error: unknown command '" << argv[1] << "'.\n";
    return 1;
}

bool TurnOffMonitor(){
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
    return true;
}
bool GetBatteryIsCharging() {
    bool isCharging;
    if (LenovoBatteryControl::GetCurrentBatteryIsCharging(isCharging)) {
        if (isCharging) {
            std::cout << "Charging\n";
        } else {
            std::cout << "Not charging\n";
        }
        return true;
    } else {
        std::cerr << "Failed to get charging status.\n";
        return false;
    }
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