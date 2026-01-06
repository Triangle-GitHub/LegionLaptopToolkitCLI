#include <Windows.h>
#include <Wbemidl.h>
#include <comdef.h>
#include <string>
#include <stdexcept>

class OverDriveController {
public:
    OverDriveController() {
        InitializeCOM();
        ConnectToWMI();
    }

    ~OverDriveController() {
        if (m_pServices) m_pServices->Release();
        if (m_pLocator) m_pLocator->Release();
        CoUninitialize();
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

    void InitializeCOM() {
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            throw std::runtime_error("COM initialization failed");
        }

        hr = CoInitializeSecurity(
            nullptr,
            -1,
            nullptr,
            nullptr,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE,
            nullptr
        );
        if (FAILED(hr)) {
            CoUninitialize();
            throw std::runtime_error("Security initialization failed");
        }
    }

    void ConnectToWMI() {
        HRESULT hr = CoCreateInstance(
            CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (LPVOID*)&m_pLocator
        );
        if (FAILED(hr)) {
            throw std::runtime_error("WbemLocator creation failed");
        }

        hr = m_pLocator->ConnectServer(
            _bstr_t(L"ROOT\\WMI"),
            nullptr,
            nullptr,
            0,
            0,
            nullptr,
            nullptr,
            &m_pServices
        );
        if (FAILED(hr)) {
            m_pLocator->Release();
            m_pLocator = nullptr;
            throw std::runtime_error("WMI connection failed");
        }

        hr = CoSetProxyBlanket(
            m_pServices,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE
        );
        if (FAILED(hr)) {
            m_pServices->Release();
            m_pServices = nullptr;
            m_pLocator->Release();
            m_pLocator = nullptr;
            throw std::runtime_error("Proxy blanket setup failed");
        }
    }

    //获取第一个 LENOVO_GAMEZONE_DATA 实例的路径
    std::wstring GetFirstInstancePath() {
        IEnumWbemClassObject* pEnumerator = nullptr;
        HRESULT hr = m_pServices->CreateInstanceEnum(
            _bstr_t(L"LENOVO_GAMEZONE_DATA"),
            WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr,
            &pEnumerator
        );
        if (FAILED(hr)) return L"";

        IWbemClassObject* pInstance = nullptr;
        ULONG uReturned = 0;
        std::wstring path;

        if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pInstance, &uReturned)) && uReturned > 0) {
            VARIANT varPath;
            VariantInit(&varPath);
            if (SUCCEEDED(pInstance->Get(L"__PATH", 0, &varPath, nullptr, nullptr)) && varPath.vt == VT_BSTR) {
                path = varPath.bstrVal;
            }
            VariantClear(&varPath);
            pInstance->Release();
        }
        pEnumerator->Release();
        return path;
    }

    int CallMethodInt(const wchar_t* methodName) {
        std::wstring instancePath = GetFirstInstancePath();
        if (instancePath.empty()) {
            return -1; // 未找到实例
        }

        IWbemClassObject* pOutParams = nullptr;
        HRESULT hr = m_pServices->ExecMethod(
            _bstr_t(instancePath.c_str()),  // 使用实际实例路径
            _bstr_t(methodName),
            0,
            nullptr,
            nullptr,  // 无输入参数
            &pOutParams,  // 接收 IWbemClassObject** 类型
            nullptr
        );

        if (FAILED(hr) || !pOutParams) {
            return -1;
        }

        // 从返回对象中提取 "Data" 属性
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
    }

    //设置方法实现
    HRESULT CallMethod(const wchar_t* methodName, int paramValue) {
        std::wstring instancePath = GetFirstInstancePath();
        if (instancePath.empty()) {
            return E_FAIL;
        }

        // 获取方法输入参数定义
        IWbemClassObject* pClass = nullptr;
        HRESULT hr = m_pServices->GetObject(
            _bstr_t(L"LENOVO_GAMEZONE_DATA"),
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

        // 创建参数实例
        IWbemClassObject* pInParams = nullptr;
        hr = pInParamsDef->SpawnInstance(0, &pInParams);
        pInParamsDef->Release();
        if (FAILED(hr)) return hr;

        // 设置参数 "Data"
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

        // 调用方法
        IWbemClassObject* pOutParams = nullptr;
        hr = m_pServices->ExecMethod(
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
    }
};