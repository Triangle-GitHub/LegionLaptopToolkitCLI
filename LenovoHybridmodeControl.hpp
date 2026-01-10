#include "CommonUtils.hpp"
#include <iostream>
#include <string>
#include <comdef.h>
#include <future>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <vector>
#include <stdexcept>

enum class IGPUModeState {
    Default = 0,
    IGPUOnly = 1,
    Auto = 2,
};

enum class HybridModeState {
    On,
    OnIGPUOnly,
    OnAuto,
    Off
};

enum class OperationResult {
    Success = 0,
    ComInitializationFailed = 1,
    WmiConnectionFailed = 2,
    InstanceNotFound = 3,
    MethodCallFailed = 4,
    InvalidMode = 5,
    DGpuEjectionFailed = 6,
    DGpuActivationFailed = 7,
    NotSupported = 8
};

class HybridModeController {
private:
    IWbemLocator* m_pLocator = nullptr;
    IWbemServices* m_pServices = nullptr;
    std::wstring m_instancePath;
    std::atomic<bool> m_stopDgpuCheck = false;
    std::mutex m_checkMutex;
    bool m_gsyncSupported = false;
    bool m_igpuModeSupported = false;
    
    HRESULT initializeWMI() {
        HRESULT hr = LenovoCommonUtils::InitializeCOMWithSecurity();
        if (FAILED(hr)) {
            return hr;
        }
        hr = LenovoCommonUtils::ConnectToWMI(&m_pLocator, &m_pServices);
        if (FAILED(hr)) {
            LenovoCommonUtils::UninitializeCOM();
            return hr;
        }
        m_instancePath = LenovoCommonUtils::GetFirstWmiInstancePathStandard(
            m_pServices, L"LENOVO_GAMEZONE_DATA");
        return S_OK;
    }
    
    void cleanupWMI() {
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
    
    bool isGSyncSupported() {
        if (m_instancePath.empty()) return false;
        int result = LenovoCommonUtils::CallWmiMethodNoParams(
            m_pServices, m_instancePath.c_str(), L"IsSupportGSync");
        return result > 0;
    }
    
    int getGSyncStatus() {
        if (m_instancePath.empty()) return -1;
        return LenovoCommonUtils::CallWmiMethodNoParams(
            m_pServices, m_instancePath.c_str(), L"GetGSyncStatus");
    }
    
    bool setGSyncStatus(bool enable) {
        if (m_instancePath.empty()) return false;
        return LenovoCommonUtils::CallWmiMethodWithIntParamFromClassDef(
            m_pServices,
            m_instancePath,
            L"LENOVO_GAMEZONE_DATA",
            L"SetGSyncStatus",
            enable ? 1 : 0
        ) == S_OK;
    }
    
    bool isIGPUModeSupported() {
        if (m_instancePath.empty()) return false;
        int result = LenovoCommonUtils::CallWmiMethodNoParams(
            m_pServices, m_instancePath.c_str(), L"IsSupportIGPUMode");
        return result > 0;
    }
    
    int getIGPUModeStatus() {
        if (m_instancePath.empty()) return -1;
        return LenovoCommonUtils::CallWmiMethodNoParams(
            m_pServices, m_instancePath.c_str(), L"GetIGPUModeStatus");
    }
    
    bool setIGPUModeStatus(IGPUModeState mode) {
        if (m_instancePath.empty()) return false;
        
        IWbemClassObject* pClass = nullptr;
        HRESULT hr = m_pServices->GetObject(
            _bstr_t(L"LENOVO_GAMEZONE_DATA"),
            WBEM_FLAG_DIRECT_READ,
            nullptr,
            &pClass,
            nullptr
        );
        if (FAILED(hr) || !pClass) {
            return false;
        }
        
        IWbemClassObject* pInParamsDef = nullptr;
        hr = pClass->GetMethod(L"SetIGPUModeStatus", 0, &pInParamsDef, nullptr);
        pClass->Release();
        if (FAILED(hr) || !pInParamsDef) {
            return false;
        }
        
        IWbemClassObject* pInParams = nullptr;
        hr = pInParamsDef->SpawnInstance(0, &pInParams);
        pInParamsDef->Release();
        if (FAILED(hr) || !pInParams) {
            return false;
        }
        
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_I4;
        var.lVal = static_cast<LONG>(mode);
        hr = pInParams->Put(L"mode", 0, &var, 0);
        VariantClear(&var);
        if (FAILED(hr)) {
            pInParams->Release();
            return false;
        }
        
        IWbemClassObject* pOutParams = nullptr;
        hr = m_pServices->ExecMethod(
            _bstr_t(m_instancePath.c_str()),
            _bstr_t(L"SetIGPUModeStatus"),
            0,
            nullptr,
            pInParams,
            &pOutParams,
            nullptr
        );
        pInParams->Release();
        if (pOutParams) {
            pOutParams->Release();
        }
        
        return SUCCEEDED(hr);
    }
    
    bool isDGPUAvailable() {
        if (m_instancePath.empty()) return false;
        int result = LenovoCommonUtils::CallWmiMethodNoParams(
            m_pServices, m_instancePath.c_str(), L"IsDGPUAvailable");
        return result > 0;
    }
    
    bool notifyDGPUStatus(bool activate) {
        if (m_instancePath.empty()) return false;
        return LenovoCommonUtils::CallWmiMethodWithIntParamFromClassDef(
            m_pServices,
            m_instancePath,
            L"LENOVO_GAMEZONE_DATA",
            L"NotifyDGPUStatus",
            activate ? 1 : 0
        ) == S_OK;
    }
    
    std::pair<bool, IGPUModeState> unpackState(HybridModeState state) {
        switch (state) {
            case HybridModeState::On:
                return {false, IGPUModeState::Default};
            case HybridModeState::OnIGPUOnly:
                return {false, IGPUModeState::IGPUOnly};
            case HybridModeState::OnAuto:
                return {false, IGPUModeState::Auto};
            case HybridModeState::Off:
                return {true, IGPUModeState::Default};
            default:
                throw std::invalid_argument("Invalid hybrid mode state");
        }
    }
    
    HybridModeState packState(bool gsyncEnabled, IGPUModeState igpuMode) {
        if (gsyncEnabled) {
            return HybridModeState::Off;
        }
        
        switch (igpuMode) {
            case IGPUModeState::Default:
                return HybridModeState::On;
            case IGPUModeState::IGPUOnly:
                return HybridModeState::OnIGPUOnly;
            case IGPUModeState::Auto:
                return HybridModeState::OnAuto;
            default:
                throw std::invalid_argument("Invalid iGPU mode");
        }
    }
    
    void ensureDGPUEjectedIfNeeded() {
        std::lock_guard<std::mutex> lock(m_checkMutex);
        m_stopDgpuCheck = false;
        
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            if (m_stopDgpuCheck) return;
            
            constexpr int maxRetries = 5;
            constexpr int delayMs = 5000;
            
            for (int retry = 1; retry <= maxRetries; ++retry) {
                if (m_stopDgpuCheck) return;
                
                if (FAILED(initializeWMI())) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    continue;
                }
                
                int currentMode = getIGPUModeStatus();
                bool isIGPUOnlyMode = (currentMode == static_cast<int>(IGPUModeState::IGPUOnly));
                
                bool isAvailable = isDGPUAvailable();
                
                cleanupWMI();
                
                if (!isIGPUOnlyMode) {
                    return;
                }
                
                if (!isAvailable) {
                    return;
                }
                
                if (SUCCEEDED(initializeWMI())) {
                    bool success = notifyDGPUStatus(false);
                    cleanupWMI();
                    if (success) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
        }).detach();
    }
    
    void ensureDGPUActivatedIfNeeded() {
        std::lock_guard<std::mutex> lock(m_checkMutex);
        m_stopDgpuCheck = false;
        
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            if (m_stopDgpuCheck) return;
            
            constexpr int maxRetries = 5;
            constexpr int delayMs = 3000;
            
            for (int retry = 1; retry <= maxRetries; ++retry) {
                if (m_stopDgpuCheck) return;
                
                if (FAILED(initializeWMI())) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    continue;
                }
                
                int currentMode = getIGPUModeStatus();
                bool isIGPUOnlyMode = (currentMode == static_cast<int>(IGPUModeState::IGPUOnly));
                
                bool isAvailable = isDGPUAvailable();
                
                cleanupWMI();
                
                if (isIGPUOnlyMode) {
                    return;
                }
                
                if (isAvailable) {
                    return;
                }
                
                if (SUCCEEDED(initializeWMI())) {
                    bool success = notifyDGPUStatus(true);
                    cleanupWMI();
                    if (success) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
        }).detach();
    }
    
    OperationResult getHybridModeInternal(HybridModeState& outMode) {
        HRESULT hr = initializeWMI();
        if (FAILED(hr)) {
            return OperationResult::ComInitializationFailed;
        }
        
        if (m_instancePath.empty()) {
            cleanupWMI();
            return OperationResult::InstanceNotFound;
        }
        
        bool gsyncEnabled = false;
        IGPUModeState igpuMode = IGPUModeState::Default;
        
        if (m_gsyncSupported) {
            int gsyncStatus = getGSyncStatus();
            gsyncEnabled = (gsyncStatus == 1);
        }
        
        if (m_igpuModeSupported) {
            int modeStatus = getIGPUModeStatus();
            if (modeStatus >= 0 && modeStatus <= 3) {
                igpuMode = static_cast<IGPUModeState>(modeStatus);
            } else {
                cleanupWMI();
                return OperationResult::MethodCallFailed;
            }
        }
        
        cleanupWMI();
        outMode = packState(gsyncEnabled, igpuMode);
        return OperationResult::Success;
    }
    
    OperationResult setHybridModeInternal(HybridModeState mode) {
        HRESULT hr = initializeWMI();
        if (FAILED(hr)) {
            return OperationResult::ComInitializationFailed;
        }
        
        if (m_instancePath.empty()) {
            cleanupWMI();
            return OperationResult::InstanceNotFound;
        }
        
        auto [targetGSync, targetIGPUMode] = unpackState(mode);
        
        {
            std::lock_guard<std::mutex> lock(m_checkMutex);
            m_stopDgpuCheck = true;
        }
        
        bool gsyncChanged = false;
        
        if (m_gsyncSupported) {
            int currentGSyncStatus = getGSyncStatus();
            bool currentGSyncEnabled = (currentGSyncStatus == 1);
            
            if (currentGSyncEnabled != targetGSync) {
                if (!setGSyncStatus(targetGSync)) {
                    cleanupWMI();
                    return OperationResult::MethodCallFailed;
                }
                gsyncChanged = true;
            }
        }
        
        if (m_igpuModeSupported) {
            int currentModeStatus = getIGPUModeStatus();
            IGPUModeState currentIGPUMode = static_cast<IGPUModeState>(currentModeStatus);
            
            if (currentIGPUMode != targetIGPUMode) {
                bool success = setIGPUModeStatus(targetIGPUMode);
                
                if (!success && !gsyncChanged) {
                    cleanupWMI();
                    return OperationResult::MethodCallFailed;
                }
                
                if (targetIGPUMode == IGPUModeState::IGPUOnly) {
                    ensureDGPUEjectedIfNeeded();
                } else if (currentIGPUMode == IGPUModeState::IGPUOnly && 
                          (targetIGPUMode == IGPUModeState::Default || targetIGPUMode == IGPUModeState::Auto)) {
                    ensureDGPUActivatedIfNeeded();
                }
            }
        }
        
        cleanupWMI();
        return OperationResult::Success;
    }

public:
    HybridModeController() {
        HRESULT hr = initializeWMI();
        if (SUCCEEDED(hr) && !m_instancePath.empty()) {
            m_gsyncSupported = isGSyncSupported();
            m_igpuModeSupported = isIGPUModeSupported();
            cleanupWMI();
        }
        m_stopDgpuCheck = false;
    }
    
    ~HybridModeController() {
        {
            std::lock_guard<std::mutex> lock(m_checkMutex);
            m_stopDgpuCheck = true;
        }
        cleanupWMI();
    }
    
    bool IsHybridModeSupported() {
        return m_gsyncSupported || m_igpuModeSupported;
    }
    
    std::vector<HybridModeState> GetSupportedStates() {
        std::vector<HybridModeState> states;
        
        if (m_gsyncSupported && m_igpuModeSupported) {
            states = {
                HybridModeState::On,
                HybridModeState::OnIGPUOnly,
                HybridModeState::OnAuto,
                HybridModeState::Off
            };
        } else if (m_igpuModeSupported) {
            states = {
                HybridModeState::On,
                HybridModeState::OnIGPUOnly,
                HybridModeState::OnAuto
            };
        } else if (m_gsyncSupported) {
            states = {
                HybridModeState::On,
                HybridModeState::Off
            };
        }
        
        return states;
    }
    
    OperationResult GetHybridModeSync(HybridModeState& outMode) {
        if (!IsHybridModeSupported()) {
            return OperationResult::NotSupported;
        }
        return getHybridModeInternal(outMode);
    }
    
    OperationResult SetHybridModeSync(HybridModeState mode) {
        if (!IsHybridModeSupported()) {
            return OperationResult::NotSupported;
        }
        
        auto supportedStates = GetSupportedStates();
        if (std::find(supportedStates.begin(), supportedStates.end(), mode) == supportedStates.end()) {
            return OperationResult::InvalidMode;
        }
        
        return setHybridModeInternal(mode);
    }
    
    std::future<std::pair<OperationResult, HybridModeState>> GetHybridModeAsync() {
        return std::async(std::launch::async, [this]() {
            HybridModeState mode;
            auto result = this->GetHybridModeSync(mode);
            return std::make_pair(result, mode);
        });
    }
    
    std::future<OperationResult> SetHybridModeAsync(HybridModeState mode) {
        return std::async(std::launch::async, [this, mode]() {
            return this->SetHybridModeSync(mode);
        });
    }
    
    bool GetGSyncStatusSync(bool& outEnabled) {
        if (!m_gsyncSupported) {
            return false;
        }
        
        HRESULT hr = initializeWMI();
        if (FAILED(hr) || m_instancePath.empty()) {
            if (SUCCEEDED(hr)) cleanupWMI();
            return false;
        }
        
        int status = getGSyncStatus();
        cleanupWMI();
        
        outEnabled = (status == 1);
        return true;
    }
    
    bool GetIGPUModeSync(IGPUModeState& outMode) {
        if (!m_igpuModeSupported) {
            return false;
        }
        
        HRESULT hr = initializeWMI();
        if (FAILED(hr) || m_instancePath.empty()) {
            if (SUCCEEDED(hr)) cleanupWMI();
            return false;
        }
        
        int mode = getIGPUModeStatus();
        cleanupWMI();
        
        if (mode < 0 || mode > 3) {
            return false;
        }
        
        outMode = static_cast<IGPUModeState>(mode);
        return true;
    }
};