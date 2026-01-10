#include <Windows.h>
#include <Wbemidl.h>
#include <comdef.h>
#include <string>
#include <stdexcept>
#include "CommonUtils.hpp"

class OverDriveController {
public:
    OverDriveController() {
        InitializeCOMAndWMI();
    }

    ~OverDriveController() {
        Cleanup();
    }

    bool IsOverDriveSupported() {
        return CallMethodInt(L"IsSupportOD") == 1;
    }

    int GetOverDriveStatus() {
        return CallMethodInt(L"GetODStatus");
    }

    HRESULT SetOverDriveStatus(bool enable) {
        return CallMethod(L"SetODStatus", enable ? 1 : 0);
    }

private:
    IWbemLocator* m_pLocator = nullptr;
    IWbemServices* m_pServices = nullptr;
    std::wstring m_instancePath;

    void InitializeCOMAndWMI() {
        HRESULT hr = LenovoCommonUtils::InitializeCOMWithSecurity();
        if (FAILED(hr)) {
            throw std::runtime_error("COM initialization failed");
        }

        hr = LenovoCommonUtils::ConnectToWMI(&m_pLocator, &m_pServices);
        if (FAILED(hr)) {
            LenovoCommonUtils::UninitializeCOM();
            throw std::runtime_error("WMI connection failed");
        }

        m_instancePath = LenovoCommonUtils::GetFirstWmiInstancePathStandard(m_pServices, L"LENOVO_GAMEZONE_DATA");
        if (m_instancePath.empty()) {
            Cleanup();
            throw std::runtime_error("Failed to get LENOVO_GAMEZONE_DATA instance path");
        }
    }

    void Cleanup() {
        if (m_pServices) {
            m_pServices->Release();
            m_pServices = nullptr;
        }
        if (m_pLocator) {
            m_pLocator->Release();
            m_pLocator = nullptr;
        }
        LenovoCommonUtils::UninitializeCOM();
    }

    int CallMethodInt(const wchar_t* methodName) {
        return LenovoCommonUtils::CallWmiMethodNoParams(m_pServices, m_instancePath, methodName);
    }

    HRESULT CallMethod(const wchar_t* methodName, int paramValue) {
        return LenovoCommonUtils::CallWmiMethodWithIntParamFromClassDef(
            m_pServices,
            m_instancePath,
            L"LENOVO_GAMEZONE_DATA",
            methodName,
            paramValue
        );
    }
};