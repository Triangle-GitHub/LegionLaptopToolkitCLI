#pragma once

#define NOMINMAX
#include <windows.h>
#include <setupapi.h>

#include <wbemidl.h>
#include <comdef.h>

#include <cstdint>
#include <vector>
#include <string>
#include <expected>

#include <wrl/client.h>

// Enums
#include "Enums.hpp"

// Declarations
namespace LLTCCommonUtils {
    // Bit & Endianness
    inline uint32_t ReverseEndianness(uint32_t value) noexcept;
    inline uint16_t ReverseEndianness16(uint16_t value) noexcept;
    inline bool GetNthBit(uint32_t value, int n) noexcept;
    
    // Drivers
    inline HANDLE GetEnergyDriverHandle() noexcept;
    inline HANDLE GetBatteryHandle() noexcept;
    inline bool GetBatteryTag(HANDLE hBattery, ULONG& outTag) noexcept;
    template<typename InputType, typename OutputType>
    inline bool EnergyDrvIoControl(
        DWORD ioctlCode,
        const InputType& input,
        OutputType& output,
        DWORD* bytesReturned = nullptr
    ) noexcept;
    
    // COM
    inline HRESULT InitializeCOM() noexcept;
    inline void UninitializeCOM() noexcept;
    
    // WMI
    enum class WmiPathType {
        Full,
        Relative
    };
    inline HRESULT ConnectToWMI(IWbemLocator** ppLocator, IWbemServices** ppServices) noexcept;
    inline std::wstring GetFirstWmiInstancePath(
        IWbemServices* pServices,
        const std::vector<std::wstring>& classNames,
        WmiPathType pathType = WmiPathType::Full
    ) noexcept;
    inline int CallWmiMethodNoParams(IWbemServices* pServices, const std::wstring& instancePath, const wchar_t* methodName) noexcept;
    inline HRESULT CallWmiMethodWithIntParamFromClassDef(
        IWbemServices* pServices, 
        const std::wstring& instancePath, 
        const wchar_t* className,
        const wchar_t* methodName, 
        int paramValue
    ) noexcept;
    inline HRESULT CallWmiMethodWithIntParamFromParameters(
        IWbemServices* pServices, 
        const std::wstring& instancePath, 
        const wchar_t* methodName, 
        int paramValue
    ) noexcept;
    inline int CallWmiMethodWithNamedParam(
        IWbemServices* pServices,
        const std::wstring& instancePath,
        const wchar_t* methodName,
        const wchar_t* paramName,
        int paramValue,
        const wchar_t* resultPropertyName = L"Value"
    ) noexcept;
}

// Definitions
namespace LLTCCommonUtils {
    inline uint32_t ReverseEndianness(uint32_t value) noexcept {
        return ((value & 0x000000FFU) << 24) |
               ((value & 0x0000FF00U) << 8)  |
               ((value & 0x00FF0000U) >> 8)  |
               ((value & 0xFF000000U) >> 24);
    }
    
    inline uint16_t ReverseEndianness16(uint16_t value) noexcept {
        return ((value & 0x00FFU) << 8) |
               ((value & 0xFF00U) >> 8);
    }
    
    inline bool GetNthBit(uint32_t value, int n) noexcept {
        return (value & (1U << n)) != 0;
    }
    
    inline HANDLE GetEnergyDriverHandle() noexcept {
        static HANDLE hDriver = INVALID_HANDLE_VALUE;
        static bool initialized = false;
        
        if (!initialized) {
            hDriver = CreateFileW(
                L"\\\\.\\EnergyDrv",
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            initialized = true;
        }
        
        return hDriver;
    }
    
    inline HANDLE GetBatteryHandle() noexcept {
        static HANDLE hBattery = INVALID_HANDLE_VALUE;
        static bool initialized = false;
        
        if (!initialized) {
            GUID guidBattery = {0x72631e54, 0x78A4, 0x11d0, {0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a}};
            HDEVINFO hDevInfo = SetupDiGetClassDevsW(
                &guidBattery,
                nullptr,
                nullptr,
                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
            );
            
            if (hDevInfo != INVALID_HANDLE_VALUE) {
                SP_DEVICE_INTERFACE_DATA devInterfaceData = {0};
                devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
                
                if (SetupDiEnumDeviceInterfaces(
                    hDevInfo,
                    nullptr,
                    &guidBattery,
                    0,
                    &devInterfaceData
                )) {
                    DWORD requiredSize = 0;
                    SetupDiGetDeviceInterfaceDetailW(
                        hDevInfo,
                        &devInterfaceData,
                        nullptr,
                        0,
                        &requiredSize,
                        nullptr
                    );
                    
                    if (requiredSize > 0) {
                        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData =
                            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);
                        
                        if (detailData) {
                            detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
                            
                            if (SetupDiGetDeviceInterfaceDetailW(
                                hDevInfo,
                                &devInterfaceData,
                                detailData,
                                requiredSize,
                                nullptr,
                                nullptr
                            )) {
                                hBattery = CreateFileW(
                                    detailData->DevicePath,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr
                                );
                            }
                            free(detailData);
                        }
                    }
                }
                SetupDiDestroyDeviceInfoList(hDevInfo);
            }
            initialized = true;
        }
        
        return hBattery;
    }
    
    inline bool GetBatteryTag(HANDLE hBattery, ULONG& outTag) noexcept {
        DWORD dwWait = 0;
        DWORD dwBytesReturned = 0;
        BOOL success = DeviceIoControl(
            hBattery,
            2703424U, // IOCTL_BATTERY_QUERY_TAG
            &dwWait,
            sizeof(dwWait),
            &outTag,
            sizeof(outTag),
            &dwBytesReturned,
            nullptr
        );
        return success && (dwBytesReturned == sizeof(outTag)) && (outTag != 0);
    }
    
    template<typename InputType, typename OutputType>
    inline bool EnergyDrvIoControl(
        DWORD ioctlCode,
        const InputType& input,
        OutputType& output,
        DWORD* bytesReturned
    ) noexcept {
        HANDLE hDriver = GetEnergyDriverHandle();
        if (hDriver == INVALID_HANDLE_VALUE) {
            return false;
        }
        
        DWORD localBytesReturned = 0;
        BOOL success = DeviceIoControl(
            hDriver,
            ioctlCode,
            const_cast<InputType*>(&input),
            sizeof(InputType),
            &output,
            sizeof(OutputType),
            bytesReturned ? bytesReturned : &localBytesReturned,
            nullptr
        );
        
        return success != FALSE;
    }

    inline HRESULT InitializeCOM() noexcept {
        return CoInitializeEx(0, COINIT_MULTITHREADED);
    }

    inline void UninitializeCOM() noexcept {
        CoUninitialize();
    }

    inline HRESULT ConnectToWMI(IWbemLocator** ppLocator, IWbemServices** ppServices) noexcept {
        try{
            HRESULT hr = CoCreateInstance(
                CLSID_WbemLocator,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemLocator,
                (LPVOID*)ppLocator
            );
            if (FAILED(hr)) {
                return hr;
            }
            
            hr = (*ppLocator)->ConnectServer(
                _bstr_t(L"ROOT\\WMI"),
                nullptr,
                nullptr,
                0,
                0,
                nullptr,
                nullptr,
                ppServices
            );
            if (FAILED(hr)) {
                (*ppLocator)->Release();
                *ppLocator = nullptr;
                return hr;
            }
            
            hr = CoSetProxyBlanket(
                *ppServices,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_NONE,
                nullptr,
                RPC_C_AUTHN_LEVEL_CALL,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                nullptr,
                EOAC_NONE
            );
            if (FAILED(hr)) {
                (*ppServices)->Release();
                *ppServices = nullptr;
                (*ppLocator)->Release();
                *ppLocator = nullptr;
                return hr;
            }
            return S_OK;
        } catch(...) {
            if (ppLocator) *ppLocator = nullptr;
            if (ppServices) *ppServices = nullptr;
            return E_UNEXPECTED;
        }
    }

    inline std::wstring GetFirstWmiInstancePath(
        IWbemServices* pServices,
        const std::vector<std::wstring>& classNames,
        WmiPathType pathType
    ) noexcept {
        try{
            if (!pServices || classNames.empty()) 
                return L"";

            const wchar_t* propName = (pathType == WmiPathType::Full) 
                ? L"__PATH" 
                : L"__RELPATH";
            
            long flags = WBEM_FLAG_RETURN_IMMEDIATELY;
            if (pathType == WmiPathType::Relative) {
                flags |= WBEM_FLAG_FORWARD_ONLY;
            }

            for (const auto& className : classNames) {
                if (className.empty()) continue;

                Microsoft::WRL::ComPtr<IEnumWbemClassObject> pEnumerator;
                HRESULT hr = pServices->CreateInstanceEnum(
                    _bstr_t(className.c_str()),
                    flags,
                    nullptr,
                    pEnumerator.GetAddressOf()
                );
                if (FAILED(hr) || !pEnumerator) 
                    continue;

                Microsoft::WRL::ComPtr<IWbemClassObject> pInstance;
                ULONG uReturned = 0;
                hr = pEnumerator->Next(WBEM_INFINITE, 1, pInstance.GetAddressOf(), &uReturned);
                if (FAILED(hr) || uReturned == 0 || !pInstance) 
                    continue;

                _variant_t vtProp;
                hr = pInstance->Get(propName, 0, &vtProp, nullptr, nullptr);
                if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR && vtProp.bstrVal) {
                    return vtProp.bstrVal;
                }
            }
            return L"";
        } catch (...) {
            return L"";
        }
    }

    inline int CallWmiMethodNoParams(IWbemServices* pServices, const std::wstring& instancePath, const wchar_t* methodName) noexcept {
        try{
            if (instancePath.empty()) {
                return -1;
            }
            
            IWbemClassObject* pOutParams = nullptr;
            HRESULT hr = pServices->ExecMethod(
                _bstr_t(instancePath.c_str()),
                _bstr_t(methodName),
                0,
                nullptr,
                nullptr,
                &pOutParams,
                nullptr
            );
            
            if (FAILED(hr) || !pOutParams) {
                return -1;
            }
            
            VARIANT var;
            VariantInit(&var);
            hr = pOutParams->Get(L"Data", 0, &var, nullptr, nullptr);
            int result = -1;
            
            if (SUCCEEDED(hr) && var.vt == VT_I4) {
                result = var.lVal;
            }
            
            VariantClear(&var);
            pOutParams->Release();
            return result;
        } catch (...) {
            return -1;
        }
    }

    inline HRESULT CallWmiMethodWithIntParamFromClassDef(
        IWbemServices* pServices, 
        const std::wstring& instancePath, 
        const wchar_t* className,
        const wchar_t* methodName, 
        int paramValue
    ) noexcept {
        try{
            if (instancePath.empty()) {
                return E_FAIL;
            }
            
            IWbemClassObject* pClass = nullptr;
            HRESULT hr = pServices->GetObject(
                _bstr_t(className),
                WBEM_FLAG_DIRECT_READ,
                nullptr,
                &pClass,
                nullptr
            );
            if (FAILED(hr)) return hr;
            
            IWbemClassObject* pInParamsDef = nullptr;
            hr = pClass->GetMethod(methodName, 0, &pInParamsDef, nullptr);
            pClass->Release();
            if (FAILED(hr) || !pInParamsDef) return hr;
            
            IWbemClassObject* pInParams = nullptr;
            hr = pInParamsDef->SpawnInstance(0, &pInParams);
            pInParamsDef->Release();
            if (FAILED(hr)) return hr;
            
            VARIANT var;
            VariantInit(&var);
            var.vt = VT_I4;
            var.lVal = paramValue;
            hr = pInParams->Put(L"Data", 0, &var, 0);
            VariantClear(&var);
            if (FAILED(hr)) {
                pInParams->Release();
                return hr;
            }
            
            IWbemClassObject* pOutParams = nullptr;
            hr = pServices->ExecMethod(
                _bstr_t(instancePath.c_str()),
                _bstr_t(methodName),
                0,
                nullptr,
                pInParams,
                &pOutParams,
                nullptr
            );
            pInParams->Release();
            if (pOutParams) pOutParams->Release();
            return hr;
        } catch (...) {
            return E_UNEXPECTED;
        }
    }

    inline HRESULT CallWmiMethodWithIntParamFromParameters(
        IWbemServices* pServices, 
        const std::wstring& instancePath, 
        const wchar_t* methodName, 
        int paramValue
    ) noexcept {
        try{
            if (instancePath.empty()) {
                return E_FAIL;
            }
            
            IWbemClassObject* pInParams = nullptr;
            HRESULT hr = pServices->GetObject(
                _bstr_t(L"__PARAMETERS"),
                0,
                nullptr,
                &pInParams,
                nullptr
            );
            if (FAILED(hr) || !pInParams) {
                return hr;
            }
            
            VARIANT vtData;
            VariantInit(&vtData);
            vtData.vt = VT_I4;
            vtData.lVal = paramValue;
            hr = pInParams->Put(L"Data", 0, &vtData, 0);
            VariantClear(&vtData);
            if (FAILED(hr)) {
                pInParams->Release();
                return hr;
            }
            
            hr = pServices->ExecMethod(
                _bstr_t(instancePath.c_str()),
                _bstr_t(methodName),
                0,
                nullptr,
                pInParams,
                nullptr,
                nullptr
            );
            pInParams->Release();
            return hr;
        } catch (...) {
            return E_UNEXPECTED;
        }
    }
    inline int CallWmiMethodWithNamedParam(
        IWbemServices* pServices,
        const std::wstring& instancePath,
        const wchar_t* methodName,
        const wchar_t* paramName,
        int paramValue,
        const wchar_t* resultPropertyName
    ) noexcept {
        try{
            if (!pServices || instancePath.empty()) return -1;
            
            IWbemClassObject* pClass = nullptr;
            HRESULT hr = pServices->GetObject(
                _bstr_t(L"__PARAMETERS"), 
                0, 
                nullptr, 
                &pClass, 
                nullptr
            );
            if (FAILED(hr) || !pClass) return -1;
            
            IWbemClassObject* pInParams = nullptr;
            hr = pClass->SpawnInstance(0, &pInParams);
            pClass->Release();
            if (FAILED(hr) || !pInParams) return -1;
            
            VARIANT vtParam;
            VariantInit(&vtParam);
            vtParam.vt = VT_I4;
            vtParam.lVal = paramValue;
            hr = pInParams->Put(paramName, 0, &vtParam, 0);
            VariantClear(&vtParam);
            if (FAILED(hr)) {
                pInParams->Release();
                return -1;
            }
            
            IWbemClassObject* pOutParams = nullptr;
            hr = pServices->ExecMethod(
                _bstr_t(instancePath.c_str()),
                _bstr_t(methodName),
                0,
                nullptr,
                pInParams,
                &pOutParams,
                nullptr
            );
            pInParams->Release();
            
            if (FAILED(hr) || !pOutParams) return -1;
            
            VARIANT vtResult;
            VariantInit(&vtResult);
            hr = pOutParams->Get(resultPropertyName, 0, &vtResult, nullptr, nullptr);
            int result = -1;
            if (SUCCEEDED(hr) && vtResult.vt == VT_I4) {
                result = vtResult.lVal;
            }
            VariantClear(&vtResult);
            pOutParams->Release();
            
            return result;
        } catch (...) {
            return -1;
        }
    }
}