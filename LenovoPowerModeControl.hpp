#pragma once

#include "CommonUtils.hpp"
#include <optional>
#include <iostream>

// Declarations
namespace LLTCPowerMode {
    inline std::expected<PowerMode, ResultState> GetState() noexcept;
    inline std::expected<void, ResultState> SetState(PowerMode mode) noexcept;
}

// Definitions
namespace LLTCPowerMode {
    namespace {
        const std::vector<std::wstring> WmiClassNames = {
            L"LENOVO_GAMEZONE_DATA",
            L"Lenovo_GameZone_Data"
        };
        std::optional<PowerMode> InternalGetPowerMode(IWbemServices* pSvc) {
            using namespace LLTCCommonUtils;
            
            auto instancePath = GetFirstWmiInstancePath(pSvc, WmiClassNames, WmiPathType::Relative);
            if (instancePath.empty()) {
                return std::nullopt;
            }
            
            IWbemClassObject* pOutParams = nullptr;
            HRESULT hr = pSvc->ExecMethod(
                _bstr_t(instancePath.c_str()),
                _bstr_t(L"GetSmartFanMode"),
                0,
                nullptr,
                nullptr,
                &pOutParams,
                nullptr
            );
            
            if (FAILED(hr) || !pOutParams) {
                return std::nullopt;
            }
            
            VARIANT vtProp;
            VariantInit(&vtProp);
            hr = pOutParams->Get(L"Data", 0, &vtProp, 0, 0);
            pOutParams->Release();
            
            if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
                int mode = vtProp.lVal;
                VariantClear(&vtProp);
                if ((mode >= 1 && mode <= 3) || mode == 254) {
                    return static_cast<PowerMode>(mode);
                }
            }
            VariantClear(&vtProp);
            
            return std::nullopt;
        }
    }   // namespace

    inline std::expected<PowerMode, ResultState> GetState() noexcept {
        HRESULT hr = LLTCCommonUtils::InitializeCOM();
        if (FAILED(hr)) {
            return std::unexpected(ResultState::Failed);
        }
        
        IWbemLocator* pLoc = nullptr;
        IWbemServices* pSvc = nullptr;
        
        hr = LLTCCommonUtils::ConnectToWMI(&pLoc, &pSvc);
        if (SUCCEEDED(hr) && pSvc) {
            auto currentMode = InternalGetPowerMode(pSvc);
            if (currentMode.has_value()) {
                return currentMode.value();
            }
        }
        LLTCCommonUtils::UninitializeCOM();
        return std::unexpected(ResultState::Failed);
    }

    inline std::expected<void, ResultState> SetState(PowerMode mode) noexcept {
        if(mode == PowerMode::GodMode)
            return std::unexpected(ResultState::NotSupported);
        
        HRESULT hr = LLTCCommonUtils::InitializeCOM();
        if (FAILED(hr)) {
            return std::unexpected(ResultState::Failed);
        }
        
        IWbemLocator* pLoc = nullptr;
        IWbemServices* pSvc = nullptr;
        
        bool result = false;
        hr = LLTCCommonUtils::ConnectToWMI(&pLoc, &pSvc);
        if (SUCCEEDED(hr) && pSvc) {
            auto instancePath = LLTCCommonUtils::GetFirstWmiInstancePath(pSvc, WmiClassNames, LLTCCommonUtils::WmiPathType::Relative);
            if (!instancePath.empty()) {
                /* 
                std::optional<PowerMode> currentMode = InternalGetPowerMode(pSvc);
                if (currentMode == PowerMode::Quiet && mode == PowerMode::Performance) {
                    if (!CallWmiMethodWithIntParamFromParameters(pSvc, instancePath, L"SetSmartFanMode", static_cast<int>(PowerMode::Balance))) {
                        goto cleanup;
                    }
                    Sleep(300);
                }
                */
                result = SUCCEEDED(LLTCCommonUtils::CallWmiMethodWithIntParamFromParameters(
                    pSvc, 
                    instancePath, 
                    L"SetSmartFanMode", 
                    static_cast<int>(mode)
                ));
            }
        }
        
        LLTCCommonUtils::UninitializeCOM();
        if(!result) 
            return std::unexpected(ResultState::Failed);
        return {};
    }

} // namespace LegionPowerMode