#pragma once
#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <vector>
#include <string>
#include <algorithm>
#include <optional>
#include <thread>
#include <chrono>
#include <stdexcept>

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

    class ComInitializer {
    public:
        ComInitializer() {
            HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (FAILED(hr)) {
                throw std::runtime_error("COM initialization failed");
            }
        }
        ~ComInitializer() {
            CoUninitialize();
        }
    };

    template <typename T>
    class ComPtr {
    public:
        ComPtr() noexcept : ptr(nullptr) {}
        ~ComPtr() { reset(); }

        void reset() noexcept {
            if (ptr) {
                ptr->Release();
                ptr = nullptr;
            }
        }

        T** operator&() noexcept {
            reset();
            return &ptr;
        }

        T* get() const noexcept { return ptr; }
        T* operator->() const noexcept { return ptr; }

        explicit operator bool() const noexcept { return ptr != nullptr; }

    private:
        T* ptr = nullptr;
    };

    std::optional<PowerMode> InternalGetPowerMode(IWbemServices* pSvc) {
        for (const auto& className : WmiClassNames) {
            ComPtr<IEnumWbemClassObject> pEnumerator;
            HRESULT hr = pSvc->CreateInstanceEnum(
                _bstr_t(className.c_str()),
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                nullptr,
                &pEnumerator
            );
            if (FAILED(hr) || !pEnumerator) continue;

            ULONG uReturn = 0;
            ComPtr<IWbemClassObject> pInstance;
            hr = pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &uReturn);
            if (FAILED(hr) || uReturn == 0 || !pInstance) continue;

            VARIANT vtPath;
            VariantInit(&vtPath);
            hr = pInstance->Get(L"__RELPATH", 0, &vtPath, 0, 0);
            if (FAILED(hr) || vtPath.vt != VT_BSTR) {
                VariantClear(&vtPath);
                continue;
            }

            ComPtr<IWbemClassObject> pInParams;
            hr = pSvc->GetObject(
                _bstr_t(L"__PARAMETERS"),
                0,
                nullptr,
                &pInParams,
                nullptr
            );
            if (FAILED(hr) || !pInParams) {
                VariantClear(&vtPath);
                continue;
            }

            ComPtr<IWbemClassObject> pOutParams;
            hr = pSvc->ExecMethod(
                vtPath.bstrVal,
                _bstr_t(L"GetSmartFanMode"),
                0,
                nullptr,
                pInParams.get(),
                &pOutParams,
                nullptr
            );
            VariantClear(&vtPath);
            if (FAILED(hr) || !pOutParams) continue;

            VARIANT vtProp;
            VariantInit(&vtProp);
            hr = pOutParams->Get(L"Data", 0, &vtProp, 0, 0);
            if (SUCCEEDED(hr) && vtProp.vt == VT_I4) {
                int mode = vtProp.lVal;
                VariantClear(&vtProp);
                if ((mode >= 1 && mode <= 3) || mode == 254) {
                    return static_cast<PowerMode>(mode);
                }
            }
            VariantClear(&vtProp);
        }
        return std::nullopt;
    }

    bool CallWmiMethod(IWbemServices* pSvc, const wchar_t* methodName, int data) {
        for (const auto& className : WmiClassNames) {
            ComPtr<IEnumWbemClassObject> pEnumerator;
            HRESULT hr = pSvc->CreateInstanceEnum(
                _bstr_t(className.c_str()),
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                nullptr,
                &pEnumerator
            );
            if (FAILED(hr) || !pEnumerator) continue;

            ULONG uReturn = 0;
            ComPtr<IWbemClassObject> pInstance;
            hr = pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &uReturn);
            if (FAILED(hr) || uReturn == 0 || !pInstance) continue;

            VARIANT vtPath;
            VariantInit(&vtPath);
            hr = pInstance->Get(L"__RELPATH", 0, &vtPath, 0, 0);
            if (FAILED(hr) || vtPath.vt != VT_BSTR) {
                VariantClear(&vtPath);
                continue;
            }

            ComPtr<IWbemClassObject> pInParams;
            hr = pSvc->GetObject(
                _bstr_t(L"__PARAMETERS"),
                0,
                nullptr,
                &pInParams,
                nullptr
            );
            if (SUCCEEDED(hr) && pInParams) {
                VARIANT vtData;
                VariantInit(&vtData);
                vtData.vt = VT_I4;
                vtData.lVal = data;
                hr = pInParams->Put(L"Data", 0, &vtData, 0);
                VariantClear(&vtData);
            }
            if (FAILED(hr) || !pInParams) {
                VariantClear(&vtPath);
                continue;
            }

            hr = pSvc->ExecMethod(
                vtPath.bstrVal,
                _bstr_t(methodName),
                0,
                nullptr,
                pInParams.get(),
                nullptr,
                nullptr
            );
            VariantClear(&vtPath);
            if (SUCCEEDED(hr)) return true;
        }
        return false;
    }
}

bool GetPowerMode(PowerMode& mode) {
    ComInitializer comInit;
    
    ComPtr<IWbemLocator> pLoc;
    HRESULT hr = CoCreateInstance(
        CLSID_WbemLocator,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (void**)&pLoc
    );
    if (FAILED(hr)) return false;
    
    ComPtr<IWbemServices> pSvc;
    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\WMI"),
        nullptr,
        nullptr,
        0,
        0,
        nullptr,
        nullptr,
        &pSvc
    );
    if (FAILED(hr)) return false;
    
    hr = CoSetProxyBlanket(
        pSvc.get(),
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE
    );
    if (FAILED(hr)) return false;
    
    auto result = InternalGetPowerMode(pSvc.get());
    if (!result.has_value()) {
        return false;
    }
    
    mode = result.value();
    return true;
}

bool SetPowerMode(PowerMode mode) {
    ComInitializer comInit;
    ComPtr<IWbemLocator> pLoc;

    HRESULT hr = CoCreateInstance(
        CLSID_WbemLocator,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (void**)&pLoc
    );
    if (FAILED(hr)) return false;

    ComPtr<IWbemServices> pSvc;
    hr = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\WMI"),
        nullptr,
        nullptr,
        0,
        0,
        nullptr,
        nullptr,
        &pSvc
    );
    if (FAILED(hr)) return false;

    hr = CoSetProxyBlanket(
        pSvc.get(),
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE
    );
    if (FAILED(hr)) return false;

    /*std::optional<PowerMode> currentMode = InternalGetPowerMode(pSvc.get());

    if (currentMode == PowerMode::Quiet && mode == PowerMode::Performance) {
        if (!CallWmiMethod(pSvc.get(), L"SetSmartFanMode", static_cast<int>(PowerMode::Balance))) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }*/

    return CallWmiMethod(pSvc.get(), L"SetSmartFanMode", static_cast<int>(mode));
}

} // namespace LegionPowerMode