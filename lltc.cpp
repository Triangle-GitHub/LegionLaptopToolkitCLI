#include<windows.h>
#include<iostream>
#include<algorithm>
#include"LenovoBatteryControl.hpp"
#include"LenovoOverdriveControl.hpp"
#include"LenovoWhitekeyboardbacklightControl.hpp"

inline std::string toLower(const std::string& s);
bool TurnOffMonitor();
bool GetBatteryMode();
bool SetBatteryMode(int tar);
bool GetBatteryIsCharging();
bool GetOverdrive();
bool SetOverdrive(int enable);
bool GetWhiteKeyboardBacklight();
bool SetWhiteKeyboardBacklight(int tar);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
            << "  lltc monitoroff | mo\n"
            << "  lltc get ischarging | ic\n"
            << "  lltc get batterymode | bm\n"
            << "  lltc get overdrive | od\n"
            << "  lltc get keyboardbacklight | kb\n"
            << "  lltc set batterymode <Conservation|Normal|RapidCharge|1|2|3>\n"
            << "  lltc set overdrive <on|off|1|0>\n"
            << "  lltc set keyboardbacklight <off|low|high|0|1|2>\n";
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
            std::cerr << "Error: 'get' requires a property (ischarging/ic, batterymode/bm, overdrive/od, keyboardbacklight/kb).\n";
            return 1;
        }
        std::string prop = toLower(argv[2]);
        if (prop == "ischarging" || prop == "ic") {
            return GetBatteryIsCharging() ? 0 : 1;
        } else if (prop == "batterymode" || prop == "bm") {
            return GetBatteryMode() ? 0 : 1;
        } else if (prop == "overdrive" || prop == "od") {
            return GetOverdrive() ? 0 : 1;
        } else if (prop == "keyboardbacklight" || prop == "kb") {
            return GetWhiteKeyboardBacklight() ? 0 : 1;
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
        std::cerr << "Error: only 'batterymode' (bm), 'keyboardbacklight' (kb) and 'overdrive' (od) can be set.\n";
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
bool GetBatteryIsCharging() {
    ChargingState isCharging;
    if (LenovoBatteryControl::GetChargingState(isCharging)) {
        switch (isCharging) {
            case ChargingState::Connected:
                std::cout << "Connected\n";
                break;
            case ChargingState::ConnectedLowWattage:
                std::cout << "Low-wattage adaper connected\n";
                break;
            case ChargingState::Disconnected:
                std::cout << "Disconnected\n";
                break;
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