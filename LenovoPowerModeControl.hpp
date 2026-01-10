#pragma once
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <vector>
#include <string>
#include <optional>
#include <thread>
#include <chrono>
#include <stdexcept>
#include "CommonUtils.hpp"

enum class PowerMode {
    Quiet = 1,
    Balance = 2,
    Performance = 3,
    GodMode = 254
};

namespace LegionPowerMode {

namespace {
    const std::vector<std::wstring> WmiClassNames = {
        L"LENOVO_GAMEZONE_DATA",
        L"Lenovo_GameZone_Data"
    };

    std::optional<PowerMode> InternalGetPowerMode(IWbemServices* pSvc) {
        using namespace LenovoCommonUtils;
        
        auto instancePath = GetFirstWmiInstancePathRel(pSvc, WmiClassNames);
        if (instancePath.empty()) {
            return std::nullopt;
        }
        
        // 调用GetSmartFanMode方法
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
}

bool GetPowerMode(PowerMode& mode) {
    using namespace LenovoCommonUtils;
    
    HRESULT hr = InitializeCOMWithoutSecurity();
    if (FAILED(hr)) {
        return false;
    }
    
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    
    bool result = false;
    hr = ConnectToWMI(&pLoc, &pSvc);
    if (SUCCEEDED(hr) && pSvc) {
        auto currentMode = InternalGetPowerMode(pSvc);
        if (currentMode.has_value()) {
            mode = currentMode.value();
            result = true;
        }
        
        pSvc->Release();
        pSvc = nullptr;
    }
    
    if (pLoc) {
        pLoc->Release();
        pLoc = nullptr;
    }
    
    UninitializeCOM();
    return result;
}

bool SetPowerMode(PowerMode mode) {
    using namespace LenovoCommonUtils;
    
    HRESULT hr = InitializeCOMWithoutSecurity();
    if (FAILED(hr)) {
        return false;
    }
    
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    
    bool result = false;
    hr = ConnectToWMI(&pLoc, &pSvc);
    if (SUCCEEDED(hr) && pSvc) {
        auto instancePath = GetFirstWmiInstancePathRel(pSvc, WmiClassNames);
        if (!instancePath.empty()) {
            /* 
            std::optional<PowerMode> currentMode = InternalGetPowerMode(pSvc);
            if (currentMode == PowerMode::Quiet && mode == PowerMode::Performance) {
                if (!CallWmiMethodWithIntParamFromParameters(pSvc, instancePath, L"SetSmartFanMode", static_cast<int>(PowerMode::Balance))) {
                    goto cleanup;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
            */
            
            result = CallWmiMethodWithIntParamFromParameters(
                pSvc, 
                instancePath, 
                L"SetSmartFanMode", 
                static_cast<int>(mode)
            );
        }
        
        // cleanup:
        pSvc->Release();
        pSvc = nullptr;
    }
    
    if (pLoc) {
        pLoc->Release();
        pLoc = nullptr;
    }
    
    UninitializeCOM();
    return result;
}

} // namespace LegionPowerMode