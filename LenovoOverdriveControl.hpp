#pragma once

#include "CommonUtils.hpp"

// Declarations
namespace LLTCOverDrive {
    inline bool IsSupported() noexcept;
    inline std::expected<OverDriveState, ResultState> GetState() noexcept;
    inline std::expected<void, ResultState> SetState(OverDriveState state) noexcept;
    inline std::expected<void, ResultState> SetState(int enable) noexcept;
}

// Definitions
namespace LLTCOverDrive {
    namespace{
        IWbemLocator* m_pLocator = nullptr;
        IWbemServices* m_pServices = nullptr;
        std::wstring m_instancePath;

        void Cleanup() noexcept{
            if (m_pServices) {
                m_pServices->Release();
                m_pServices = nullptr;
            }
            if (m_pLocator) {
                m_pLocator->Release();
                m_pLocator = nullptr;
            }
            LLTCCommonUtils::UninitializeCOM();
        }

        void InitializeCOMAndWMI() {
            HRESULT hr = LLTCCommonUtils::InitializeCOM();
            if (FAILED(hr)) {
                throw std::runtime_error("COM initialization failed");
            }

            hr = LLTCCommonUtils::ConnectToWMI(&m_pLocator, &m_pServices);
            if (FAILED(hr)) {
                LLTCCommonUtils::UninitializeCOM();
                throw std::runtime_error("WMI connection failed");
            }

            m_instancePath = LLTCCommonUtils::GetFirstWmiInstancePath(m_pServices, {L"LENOVO_GAMEZONE_DATA"}, LLTCCommonUtils::WmiPathType::Full);
            if (m_instancePath.empty()) {
                Cleanup();
                throw std::runtime_error("Failed to get LENOVO_GAMEZONE_DATA instance path");
            }
        }

        int CallMethodInt(const wchar_t* methodName) noexcept{
            return LLTCCommonUtils::CallWmiMethodNoParams(m_pServices, m_instancePath, methodName);
        }

        HRESULT CallMethod(const wchar_t* methodName, int paramValue) noexcept{
            return LLTCCommonUtils::CallWmiMethodWithIntParamFromClassDef(
                m_pServices,
                m_instancePath,
                L"LENOVO_GAMEZONE_DATA",
                methodName,
                paramValue
            );
        }
    }
    inline bool IsSupported() noexcept {
        try{
            InitializeCOMAndWMI();
        } catch (...) {
            return false;
        }
        return CallMethodInt(L"IsSupportOD") == 1;
    }

    inline std::expected<OverDriveState, ResultState> GetState() noexcept {
        if(!IsSupported())
            return std::unexpected(ResultState::NotSupported);
        auto result = intToOverDriveState(CallMethodInt(L"GetODStatus"));
        if(!result)
            return std::unexpected(ResultState::Failed);
        return result.value();
    }

    inline std::expected<void, ResultState> SetState(OverDriveState state) noexcept {
        if(!IsSupported())
            return std::unexpected(ResultState::NotSupported);
        if(!SUCCEEDED(CallMethod(L"SetODStatus", (state == OverDriveState::On) ? 1 : 0)))
            return std::unexpected(ResultState::Failed);
        return {};
    }

    inline std::expected<void, ResultState> SetState(int enable) noexcept {
        auto result = intToOverDriveState(enable);
        if(!result)
            return std::unexpected(ResultState::InvalidParameter);
        return SetState(result.value());
    }
};