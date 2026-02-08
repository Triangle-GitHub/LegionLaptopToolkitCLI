enum class ResultState {
    Success = 0,
    Failed,
    InvalidParameter,
    RetryTimeout,
    NotSupported
};
constexpr std::string_view to_string(ResultState state) noexcept {
    switch (state) {
        case ResultState::Success:          return "Success";
        case ResultState::Failed:           return "Failed";
        case ResultState::InvalidParameter: return "Invalid parameter(s)";
        case ResultState::RetryTimeout:     return "Maximum retry attempts reached";
        case ResultState::NotSupported:     return "Not supported or permission denined";
        default:                            return "Unknown error";
    }
}

enum class AlwaysOnUSBState {
    Off = 0,
    OnWhenSleeping = 1,
    OnAlways = 2
};
constexpr std::string_view to_string(AlwaysOnUSBState state) noexcept {
    switch (state) {
        case AlwaysOnUSBState::Off:            return "Off";
        case AlwaysOnUSBState::OnWhenSleeping: return "On, when sleeping";
        case AlwaysOnUSBState::OnAlways:       return "On, always";
        default:                               return "Unknown";
    }
}
std::expected<AlwaysOnUSBState, ResultState> intToAlwaysOnUSBState(int value) noexcept {
    switch (value) {
        case 0:  return AlwaysOnUSBState::Off;
        case 1:  return AlwaysOnUSBState::OnWhenSleeping;
        case 2:  return AlwaysOnUSBState::OnAlways;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class OverDriveState {
    Off = 0,
    On = 1
};
constexpr std::string_view to_string(OverDriveState state) noexcept {
    switch (state) {
        case OverDriveState::Off:   return "Off";
        case OverDriveState::On:    return "On";
        default:                    return "Unknown";
    }
}
std::expected<OverDriveState, ResultState> intToOverDriveState(int value) noexcept {
    switch (value) {
        case 0:  return OverDriveState::Off;
        case 1:  return OverDriveState::On;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class WhiteKeyboardBacklightState {
    Off = 0,
    Low = 1,
    High = 2
};
constexpr std::string_view to_string(WhiteKeyboardBacklightState state) noexcept {
    switch (state) {
        case WhiteKeyboardBacklightState::Off:  return "Off";
        case WhiteKeyboardBacklightState::Low:  return "Low";
        case WhiteKeyboardBacklightState::High: return "High";
        default:                                return "Unknown";
    }
}
std::expected<WhiteKeyboardBacklightState, ResultState> intToWhiteKeyboardBacklightState(int value) noexcept {
    switch (value) {
        case 0:  return WhiteKeyboardBacklightState::Off;
        case 1:  return WhiteKeyboardBacklightState::Low;
        case 2:  return WhiteKeyboardBacklightState::High;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class PowerMode {
    Quiet = 1,
    Balance = 2,
    Performance = 3,
    GodMode = 254
};
constexpr std::string_view to_string(PowerMode state) noexcept {
    switch (state) {
        case PowerMode::Quiet:          return "Quiet";
        case PowerMode::Balance:        return "Balance";
        case PowerMode::Performance:    return "Performance";
        case PowerMode::GodMode:        return "GodMode";
        default:                        return "Unknown";
    }
}
std::expected<PowerMode, ResultState> intToPowerMode(int value) noexcept {
    switch (value) {
        case 1:     return PowerMode::Quiet;
        case 2:     return PowerMode::Balance;
        case 3:     return PowerMode::Performance;
        case 254:   return PowerMode::GodMode;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class BatteryMode {
    Conservation,
    Normal,
    RapidCharge
};
constexpr std::string_view to_string(BatteryMode state) noexcept {
    switch (state) {
        case BatteryMode::Conservation:    return "Conservation";
        case BatteryMode::Normal:          return "Normal";
        case BatteryMode::RapidCharge:     return "RapidCharge";
        default:                           return "Unknown";
    }
}
std::expected<BatteryMode, ResultState> intToBatteryMode(int value) noexcept {
    switch (value) {
        case 1:  return BatteryMode::Conservation;
        case 2:  return BatteryMode::Normal;
        case 3:  return BatteryMode::RapidCharge;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class ChargingState {
    Connected,
    ConnectedLowWattage,
    Disconnected
};
constexpr std::string_view to_string(ChargingState state) noexcept {
    switch (state) {
        case ChargingState::Connected:              return "Adapter connected";
        case ChargingState::ConnectedLowWattage:    return "Low-wattage adapter connected";
        case ChargingState::Disconnected:           return "Adapter disconnected";
        default:                                    return "Unknown";
    }
}
std::expected<ChargingState, ResultState> intToChargingState(int value) noexcept {
    switch (value) {
        case 1:  return ChargingState::Connected;
        case 2:  return ChargingState::ConnectedLowWattage;
        case 3:  return ChargingState::Disconnected;
        default: return std::unexpected(ResultState::InvalidParameter);
    }
}

enum class HybridModeState {
    On,
    OnIGPUOnly,
    OnAuto,
    Off
};
constexpr std::string_view to_string(HybridModeState state) noexcept {
    switch (state) {
        case HybridModeState::On:           return "Hybrid";
        case HybridModeState::OnIGPUOnly:   return "Hybrid-iGPU";
        case HybridModeState::OnAuto:       return "Hybrid-Auto";
        case HybridModeState::Off:          return "dGPU";
        default:                            return "Unknown";
    }
}
std::expected<HybridModeState, ResultState> intToHybridModeState(int value) noexcept {
    switch (value) {
        case 1:     return HybridModeState::On;
        case 2:     return HybridModeState::OnIGPUOnly;
        case 3:     return HybridModeState::OnAuto;
        case 4:     return HybridModeState::Off;
        default:    return std::unexpected(ResultState::InvalidParameter);
    }
}