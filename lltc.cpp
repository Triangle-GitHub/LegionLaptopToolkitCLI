#include "LenovoBatteryControl.hpp"
#include "LenovoOverdriveControl.hpp"
#include "LenovoWhitekeyboardbacklightControl.hpp"
#include "LenovoPowerModeControl.hpp"
#include "LenovoHybridmodeControl.hpp"
#include "LenovoAlwaysonusbControl.hpp"

#include <iomanip>
#include <print>
#include <algorithm>
#include <numeric>
#include <conio.h>

inline std::string toLower(std::string_view sv);
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
bool GetAlwaysOnUSB();
bool SetAlwaysOnUSB(int tar);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::print("Usage:\n"
                   "  lltc monitoroff | mo\n"
                   "  lltc get batterymode | bm\n"
                   "  lltc get overdrive | od\n"
                   "  lltc get keyboardbacklight | kb\n"
                   "  lltc get batteryinformation | bi\n"
                   "  lltc get batteryinformation -dmon\n"
                   "  lltc get powermode | pm\n"
                   "  lltc get gpumode | gm\n"
                   "  lltc get alwaysonusb | ao\n"
                   "  lltc set batterymode <Conservation|Normal|RapidCharge|1|2|3>\n"
                   "  lltc set overdrive <on|off|1|0>\n"
                   "  lltc set keyboardbacklight <off|low|high|0|1|2>\n"
                   "  lltc set powermode <Quiet|Balance|Performance|GodMode|1|2|3|254>\n"
                   "  lltc set gpumode <Hybrid|HybridIGPU|HybridAuto|dGPU|1|2|3|4>\n"
                   "  lltc set alwaysonusb <Off|OnWhenSleeping|OnAlways|0|1|2>\n");
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
            std::print(stderr, "Error: 'get' requires a property (batterymode/bm, overdrive/od, keyboardbacklight/kb, batteryinformation/bi, powermode/pm, gpumode/gm).\n");
            return 1;
        }
        std::string prop = toLower(argv[2]);
        
        if (prop == "batteryinformation" || prop == "bi") {
            if (argc >= 4 && toLower(argv[3]) == "-dmon") {
                int refreshS = 0;
                if (argc >= 5) {
                    try {
                        refreshS = std::stoi(argv[4]);
                        if (refreshS < 1) {
                            std::print(stderr, "Error: refresh interval must be at least 1s.\n");
                            return 1;
                        }
                    } catch (...) {
                        std::print(stderr, "Error: invalid refresh interval '{}'. Must be a number >= 1.\n", argv[4]);
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
        } else if (prop == "alwaysonusb" || prop == "ao") {
            return GetAlwaysOnUSB() ? 0 : 1;
        } else {
            std::print(stderr, "Error: unknown property '{}'.\n", argv[2]);
            return 1;
        }
    }
    // === lltc set ... ===
    if (cmd1 == "set") {
        if (argc < 3) {
            std::print(stderr, "Error: 'set' requires a property.\n");
            return 1;
        }
        std::string prop = toLower(argv[2]);

        // --- Always on USB ---
        if (prop == "alwaysonusb" || prop == "ao") {
            if (argc < 4) {
                std::print(stderr, "Error: missing AlwaysOnUSB value.\n");
                return 1;
            }
            std::string value = toLower(argv[3]);
            int modeInt = -1;
            
            try {
                size_t pos;
                int num = std::stoi(value, &pos);
                if (pos == value.size() && num >= 0 && num <= 2) {
                    modeInt = num;
                }
            } catch (...) {
                // not a number
            }
            
            if (modeInt == -1) {
                if (value == "off") {
                    modeInt = 0;
                } else if (value == "onwhensleeping" || value == "sleeping") {
                    modeInt = 1;
                } else if (value == "onalways" || value == "always") {
                    modeInt = 2;
                }
            }
            
            if (modeInt == -1) {
                std::print(stderr, "Error: invalid AlwaysOnUSB mode '{}'. Use Off/OnWhenSleeping/OnAlways or 0/1/2.\n", argv[3]);
                return 1;
            }
            return SetAlwaysOnUSB(modeInt) ? 0 : 1;
        }
        
        // --- GPU Mode ---
        if (prop == "gpumode" || prop == "gm") {
            if (argc < 4) {
                std::print(stderr, "Error: missing GPU mode value.\n");
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
                std::print(stderr, "Error: invalid GPU mode '{}'. Use Hybrid/HybridIGPU/HybridAuto/dGPU or 1/2/3/4.\n", argv[3]);
                return 1;
            }
            return SetGPUMode(modeInt) ? 0 : 1;
        }
        
        // --- Power Mode ---
        if (prop == "powermode" || prop == "pm") {
            if (argc < 4) {
                std::print(stderr, "Error: missing power mode value.\n");
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
                std::print(stderr, "Error: invalid power mode '{}'. Use Quiet/Balance/Performance/GodMode or 1/2/3/254.\n", argv[3]);
                return 1;
            }
            return SetPowerMode(modeInt) ? 0 : 1;
        }
        
        // --- Battery Mode ---
        if (prop == "batterymode" || prop == "bm") {
            if (argc < 4) {
                std::print(stderr, "Error: missing battery mode value.\n");
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
                    std::print(stderr, "Error: invalid battery mode '{}'. Use Conservation/Normal/RapidCharge or 1/2/3.\n", argv[3]);
                    return 1;
                }
            }
            return SetBatteryMode(modeInt) ? 0 : 1;
        }
        // --- Keyboard Backlight ---
        if (prop == "keyboardbacklight" || prop == "kb") {
            if (argc < 4) {
                std::print(stderr, "Error: missing keyboard backlight value.\n");
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
                    std::print(stderr, "Error: invalid keyboard backlight level '{}'. Use off/low/high or 0/1/2.\n", argv[3]);
                    return 1;
                }
            }
            return SetWhiteKeyboardBacklight(levelInt) ? 0 : 1;
        }
        // --- Overdrive (od / overdrive) ---
        if (prop == "overdrive" || prop == "od") {
            if (argc < 4) {
                std::print(stderr, "Error: missing overdrive value (on/off/1/0).\n");
                return 1;
            }
            std::string value = toLower(argv[3]);
            int enable = -1;
            if (value == "on" || value == "1") {
                enable = 1;
            } else if (value == "off" || value == "0") {
                enable = 0;
            } else {
                std::print(stderr, "Error: invalid overdrive value '{}'. Use on/off or 1/0.\n", argv[3]);
                return 1;
            }
            return SetOverdrive(enable) ? 0 : 1;
        }
        // --- Unknown property ---
        std::print(stderr, "Error: only 'powermode' (pm), 'batterymode' (bm), 'keyboardbacklight' (kb), "
                  "'overdrive' (od), 'gpumode' (gm), and 'alwaysonusb' (ao) can be set.\n");
        return 1;
    }
    std::print(stderr, "Error: unknown command '{}'.\n", argv[1]);
    return 1;
}

inline std::string toLower(std::string_view sv) {
    std::string r(sv);
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return r;
}

bool TurnOffMonitor(){
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
    return true;
}

bool GetBatteryMode(){
    auto result = LLTCBatteryControl::GetBatteryMode();
    if (result) {
        std::print("Battery charging mode: {}\n", to_string(result.value()));
    } else {
        std::print(stderr, "Failed to get battery charging mode: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}

bool SetBatteryMode(int tar){
    auto state = intToBatteryMode(tar);
    if(!state)
        return false;
    auto result = LLTCBatteryControl::SetBatteryMode(state.value());
    if(result){
        std::print("Successfully set battery charging mode to: {}\n", to_string(state.value()));
    } else {
        std::print(stderr, "Failed to set battery charging mode: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}

bool GetOverdrive() {
    auto result = LLTCOverDrive::GetState();
    if (result) {
        std::print("OverDrive state: {}\n", to_string(result.value()));
    } else {
        std::print(stderr, "Failed to get OverDrive state: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}
bool SetOverdrive(int enable) {
    auto result = LLTCOverDrive::SetState(enable);
    if(result){
        std::print("Successfully set OverDrive state to: {}\n", to_string(intToOverDriveState(enable).value()));
    } else {
        std::print(stderr, "Failed to set OverDrive state: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}

bool GetWhiteKeyboardBacklight() {
    auto result = LLTCWhiteKeyboardBacklight::GetState();
    if(result){
        std::print("Keyboard backlight state: {}\n", to_string(result.value()));
    } else {
        std::print(stderr, "Failed to get keyboard backlight state: {}\n", to_string(result.error()));
    }
    return true;
}

bool SetWhiteKeyboardBacklight(int tar) {
    auto state = intToWhiteKeyboardBacklightState(tar);
    if(!state)
        return false;
    auto result = LLTCWhiteKeyboardBacklight::SetState(state.value());
    if(result){
        std::print("Successfully set keyboard backlight state to: {}\n", to_string(state.value()));
    } else {
        std::print(stderr, "Failed to set keyboard backlight state: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}

bool GetFullBatteryInfo() {
    auto res = LLTCBatteryControl::GetBatteryInformation();
    if(!res.has_value()){
        std::print(stderr, "Failed to get battery information: {}\n", to_string(res.error()));
        return false;
    }
    const auto& result = res.value();

    std::print("AC Connected: {}\n", result.isAcConnected ? "Yes" : "No");
    std::print("Battery Life: {}%\n", static_cast<int>(result.batteryLifePercent));
    
    if (result.batteryLifeTime != 0xFFFFFFFF) {
        std::print("Estimated Remaining Time: {}h {}m\n",
            result.batteryLifeTime / 3600,
            (result.batteryLifeTime % 3600) / 60);
    }
    
    std::print("Discharge Rate: {} mW\n", result.dischargeRate);
    std::print("Current Capacity: {} mWh\n", result.currentCapacity);
    std::print("Designed Capacity: {} mWh\n", result.designedCapacity);
    std::print("Full Charged Capacity: {} mWh\n", result.fullChargedCapacity);
    std::print("Cycle Count: {}\n", result.cycleCount);
    std::print("Low Battery Alert: {}\n", result.isLowBattery ? "Yes" : "No");

    std::print("======\n");
    
    if (result.temperatureC >= 0) {
        std::print("Battery Temperature: {:.1f} C\n", result.temperatureC);
    } else {
        std::print("Battery Temperature: Not available\n");
    }

    if (result.manufactureDate.wYear != 0) {
        std::print("Manufacture Date: {}-{:02}-{:02}\n",
            result.manufactureDate.wYear,
            result.manufactureDate.wMonth,
            result.manufactureDate.wDay);
    } else {
        std::print("Manufacture Date: Not available\n");
    }

    if (result.firstUseDate.wYear != 0) {
        std::print("First Use Date: {}-{:02}-{:02}\n",
            result.firstUseDate.wYear,
            result.firstUseDate.wMonth,
            result.firstUseDate.wDay);
    } else {
        std::print("First Use Date: Not available\n");
    }
    return true;
}

void GetFullBatteryInfoDmon(int seconds) {
    GetFullBatteryInfo();
    std::print("======\n");
    
    constexpr int TIME_COL = 20;
    constexpr int DATA_COL = 8;
    
    bool dft = false;
    if (seconds == 0) {
        seconds = 1;
        dft = true;
        std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
            " ", TIME_COL,
            "AC", DATA_COL,
            "temp", DATA_COL,
            "pct", DATA_COL,
            "pwr", DATA_COL,
            "cap", DATA_COL,
            "cycle", DATA_COL,
            "low", DATA_COL
        );
        std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
            " ", TIME_COL,
            "", DATA_COL,
            "(C)", DATA_COL,
            "(%)", DATA_COL,
            "(W)", DATA_COL,
            "(Wh)", DATA_COL,
            "(s)", DATA_COL,
            "(Y/N)", DATA_COL
        );
    } else {
        std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
            " ", TIME_COL,
            "AC", DATA_COL,
            "temp", DATA_COL,
            "tempAvg", DATA_COL,
            "percent", DATA_COL,
            "power", DATA_COL,
            "pwrAvg", DATA_COL,
            "cap", DATA_COL,
            "cycle", DATA_COL,
            "lowCap", DATA_COL
        );
        std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
            " ", TIME_COL,
            "", DATA_COL,
            "(C)", DATA_COL,
            "(C)", DATA_COL,
            "(%)", DATA_COL,
            "(W)", DATA_COL,
            "(W)", DATA_COL,
            "(Wh)", DATA_COL,
            "(s)", DATA_COL,
            "(Y/N)", DATA_COL
        );
    }

    int count = 0;
    std::vector<double> tempSamples;
    std::vector<double> powerSamples;

    while (true) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        std::string timeStr = std::format(
            "{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
            static_cast<int>(st.wYear),
            static_cast<int>(st.wMonth),
            static_cast<int>(st.wDay),
            static_cast<int>(st.wHour),
            static_cast<int>(st.wMinute),
            static_cast<int>(st.wSecond)
        );

        auto res = LLTCBatteryControl::GetBatteryInformation();
        if(!res.has_value()){
            Sleep(1000);
            continue;
        }
        const auto& result = res.value();

        double currentTemp = result.temperatureC;
        double currentPower = result.dischargeRate / 1000.0;
        double capWh = result.currentCapacity / 1000.0;

        if (!dft) {
            if (currentTemp >= 0) tempSamples.push_back(currentTemp);
            powerSamples.push_back(currentPower);
        }

        if (seconds == 1 || count == seconds - 1) {
            double avgTemp = -1.0;
            double avgPower = 0.0;
            
            if (!dft && !tempSamples.empty()) {
                avgTemp = std::accumulate(tempSamples.begin(), tempSamples.end(), 0.0) / tempSamples.size();
            }
            if (!dft && !powerSamples.empty()) {
                avgPower = std::accumulate(powerSamples.begin(), powerSamples.end(), 0.0) / powerSamples.size();
            }

            std::string acStr = result.isAcConnected ? "Y" : "N";
            std::string tempStr = (currentTemp >= 0) 
                ? std::format("{:.1f}", currentTemp) 
                : "N/A";
            std::string pctStr = std::to_string(static_cast<int>(result.batteryLifePercent));
            std::string pwrStr = std::format("{:+.2f}", currentPower);
            std::string capStr = std::format("{:.2f}", capWh);
            std::string cycleStr = std::to_string(result.cycleCount);
            std::string lowStr = result.isLowBattery ? "Y" : "N";

            if (dft) {
                std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
                    timeStr, TIME_COL,
                    acStr, DATA_COL,
                    tempStr, DATA_COL,
                    pctStr, DATA_COL,
                    pwrStr, DATA_COL,
                    capStr, DATA_COL,
                    cycleStr, DATA_COL,
                    lowStr, DATA_COL
                );
            } else {
                std::string avgTempStr = (avgTemp >= 0)
                    ? std::format("{:.3f}", avgTemp)
                    : "N/A";
                std::string avgPwrStr = std::format("{:+.3f}", avgPower);

                std::print("{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}{:>{}s}\n",
                    timeStr, TIME_COL,
                    acStr, DATA_COL,
                    tempStr, DATA_COL,
                    avgTempStr, DATA_COL,
                    pctStr, DATA_COL,
                    pwrStr, DATA_COL,
                    avgPwrStr, DATA_COL,
                    capStr, DATA_COL,
                    cycleStr, DATA_COL,
                    lowStr, DATA_COL
                );
                
                tempSamples.clear();
                powerSamples.clear();
            }
            count = 0;
        } else {
            count++;
        }

        Sleep(1000);
    }
}

bool GetPowerMode() {
    auto result = LLTCPowerMode::GetState();
    if (result) {
        std::print("Power mode: {}\n", to_string(result.value()));
    } else {
        std::print(stderr, "Failed to get power mode: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}

bool SetPowerMode(int tar) {
    auto state = intToPowerMode(tar);
    if(!state)
        return false;
    auto result = LLTCPowerMode::SetState(state.value());
    if(result){
        std::print("Successfully set power mode to: {}\n", to_string(state.value()));
    } else {
        std::print(stderr, "Failed to set power mode: {}\n", to_string(result.error()));
        return false;
    }
    return true;
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
                std::print("Hybrid\n");
                break;
            case HybridModeState::OnIGPUOnly:
                std::print("Hybrid-iGPU\n");
                break;
            case HybridModeState::OnAuto:
                std::print("Hybrid-Auto\n");
                break;
            case HybridModeState::Off:
                std::print("dGPU\n");
                break;
        }
        return true;
    } else {
        std::print(stderr, "Failed to get current GPU mode.\n");
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
            std::print(stderr, "Invalid GPU mode.\n");
            return false;
    }
    
    auto future_get = controller.GetHybridModeAsync();
    auto [result_get, currentState] = future_get.get();
    if (result_get != OperationResult::Success) {
        std::print(stderr, "Failed to get current GPU mode.\n");
        return false;
    }

    if (currentState == targetMode) {
        std::print("Already in mode: ");
        switch (targetMode) {
            case HybridModeState::On: std::print("Hybrid\n"); break;
            case HybridModeState::OnIGPUOnly: std::print("Hybrid-iGPU\n"); break;
            case HybridModeState::OnAuto: std::print("Hybrid-Auto\n"); break;
            case HybridModeState::Off: std::print("dGPU\n"); break;
        }
        return true;
    }

    auto future_set = controller.SetHybridModeAsync(targetMode);
    OperationResult result_set = future_set.get();
    if (result_set != OperationResult::Success) {
        std::print(stderr, "Switch failed.\n");
        return false;
    }

    const bool switchingToDGpu = (targetMode == HybridModeState::Off);
    const bool switchingFromDGpu = (currentState == HybridModeState::Off);
    const bool requiresReboot = switchingToDGpu || switchingFromDGpu;

    switch (targetMode) {
        case HybridModeState::On: std::print("Hybrid\n"); break;
        case HybridModeState::OnIGPUOnly: std::print("Hybrid-iGPU\n"); break;
        case HybridModeState::OnAuto: std::print("Hybrid-Auto\n"); break;
        case HybridModeState::Off: std::print("dGPU\n"); break;
    }

    if (requiresReboot) {
        std::print("\n*** SYSTEM RESTART REQUIRED ***\n");
        std::print("Press ANY KEY to restart immediately...\n");
        _getch();
        
        std::system("shutdown /r /t 0");
        return true;
    }
    return true;
}

bool GetAlwaysOnUSB() {
    auto result = LLTCAlwaysOnUSB::GetState();
    if (result) {
        std::print("AlwaysOnUSB state: {}\n", to_string(result.value()));
    } else {
        std::print(stderr, "Failed to get AlwaysOnUSB state: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}
bool SetAlwaysOnUSB(int tar) {
    auto state = intToAlwaysOnUSBState(tar);
    if(!state)
        return false;
    auto result = LLTCAlwaysOnUSB::SetState(state.value());
    if(result){
        std::print("Successfully set AlwaysOnUSB state to: {}\n", to_string(state.value()));
    } else {
        std::print(stderr, "Failed to set AlwaysOnUSB state: {}\n", to_string(result.error()));
        return false;
    }
    return true;
}