#include "ServicesModule.h"
#include "Logger.h"
#include <winsvc.h>

std::vector<ServiceInfo> ServicesModule::GetServices() {
    std::vector<ServiceInfo> services;
    
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCManager) return services;
    
    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;
    
    // Получаем размер буфера
    EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                         SERVICE_STATE_ALL, NULL, 0, &bytesNeeded,
                         &servicesReturned, &resumeHandle, NULL);
    
    if (GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(hSCManager);
        return services;
    }
    
    // Выделяем буфер
    LPENUM_SERVICE_STATUS_PROCESS pServices = (LPENUM_SERVICE_STATUS_PROCESS)new BYTE[bytesNeeded];
    
    if (EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                             SERVICE_STATE_ALL, (LPBYTE)pServices, bytesNeeded,
                             &bytesNeeded, &servicesReturned, &resumeHandle, NULL)) {
        
        for (DWORD i = 0; i < servicesReturned; i++) {
            ServiceInfo info;
            info.name = pServices[i].lpServiceName;
            info.displayName = pServices[i].lpDisplayName;
            
            // Статус службы
            switch (pServices[i].ServiceStatusProcess.dwCurrentState) {
                case SERVICE_RUNNING: info.status = L"Работает"; break;
                case SERVICE_STOPPED: info.status = L"Остановлена"; break;
                case SERVICE_PAUSED: info.status = L"Приостановлена"; break;
                case SERVICE_START_PENDING: info.status = L"Запускается"; break;
                case SERVICE_STOP_PENDING: info.status = L"Останавливается"; break;
                default: info.status = L"Неизвестно";
            }
            
            // Тип запуска
            SC_HANDLE hService = OpenService(hSCManager, pServices[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (hService) {
                DWORD bytesNeeded = 0;
                QueryServiceConfig(hService, NULL, 0, &bytesNeeded);
                
                LPQUERY_SERVICE_CONFIG pConfig = (LPQUERY_SERVICE_CONFIG)new BYTE[bytesNeeded];
                
                if (QueryServiceConfig(hService, pConfig, bytesNeeded, &bytesNeeded)) {
                    switch (pConfig->dwStartType) {
                        case SERVICE_AUTO_START: info.startType = L"Автоматически"; break;
                        case SERVICE_DEMAND_START: info.startType = L"Вручную"; break;
                        case SERVICE_DISABLED: info.startType = L"Отключена"; break;
                        case SERVICE_BOOT_START: info.startType = L"Загрузочная"; break;
                        case SERVICE_SYSTEM_START: info.startType = L"Системная"; break;
                        default: info.startType = L"Неизвестно";
                    }
                    
                    if (pConfig->lpDescription) {
                        info.description = pConfig->lpDescription;
                    }
                }
                
                delete[] pConfig;
                CloseServiceHandle(hService);
            }
            
            // Проверка на возможность оптимизации
            info.canOptimize = CanOptimizeService(info);
            
            services.push_back(info);
        }
    }
    
    delete[] pServices;
    CloseServiceHandle(hSCManager);
    
    return services;
}

bool ServicesModule::CanOptimizeService(const ServiceInfo& service) {
    // Список служб, которые можно отключить для повышения производительности
    const wchar_t* optimizableServices[] = {
        L"Xbox", L"Xbl", L"lfsvc", L"WbioSrvc", L"Fax", L"PhoneSvc",
        L"PcaSvc", L"WMPNetworkSvc", L"stisvc", L"WpnService", L"MapsBroker",
        L"PimIndexMaintenanceSvc", L"UnistoreSvc", L"UserDataSvc", L"OneSyncSvc",
        L"BcastDVRUserService", L"MessagingService", L"PcaSvc", L"wcncsvc",
        L"WlanSvc", L"WlanSvc", L"WlanSvc", L"icssvc", L"wlidsvc", L"WSearch"
    };
    
    std::wstring nameLower = service.name;
    for (auto& c : nameLower) c = towlower(c);
    
    for (const wchar_t* opt : optimizableServices) {
        std::wstring optLower = opt;
        for (auto& c : optLower) c = towlower(c);
        
        if (nameLower.find(optLower) != std::wstring::npos) {
            return true;
        }
    }
    
    return false;
}

bool ServicesModule::SetServiceStartType(const std::wstring& serviceName, DWORD startType) {
    Logger::Log(L"Изменение типа запуска службы: " + serviceName, LOG_INFO);
    
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) return false;
    
    SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_CHANGE_CONFIG);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }
    
    bool success = ChangeServiceConfig(hService, SERVICE_NO_CHANGE, startType,
                                        SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    if (success) {
        Logger::Log(L"Тип запуска изменен для: " + serviceName, LOG_SUCCESS);
    } else {
        Logger::Log(L"Ошибка изменения типа запуска для: " + serviceName, LOG_ERROR);
    }
    
    return success;
}

bool ServicesModule::ControlService(const std::wstring& serviceName, DWORD control) {
    Logger::Log(L"Управление службой: " + serviceName + L" код: " + std::to_wstring(control), LOG_INFO);
    
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) return false;
    
    SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_STOP | SERVICE_START);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }
    
    SERVICE_STATUS status;
    bool success = false;
    
    if (control == SERVICE_CONTROL_STOP) {
        success = ControlService(hService, SERVICE_CONTROL_STOP, &status);
    } else if (control == SERVICE_CONTROL_CONTINUE) {
        success = ControlService(hService, SERVICE_CONTROL_CONTINUE, &status);
    }
    
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    return success;
}

bool ServicesModule::StartService(const std::wstring& serviceName) {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) return false;
    
    SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_START);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }
    
    bool success = ::StartService(hService, 0, NULL);
    
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    return success;
}

bool ServicesModule::StopService(const std::wstring& serviceName) {
    return ControlService(serviceName, SERVICE_CONTROL_STOP);
}

void ServicesModule::OptimizeServices() {
    Logger::Log(L"Оптимизация служб", LOG_INFO);
    
    auto services = GetServices();
    int optimized = 0;
    
    for (const auto& service : services) {
        if (service.canOptimize && service.startType == L"Автоматически") {
            if (SetServiceStartType(service.name, SERVICE_DEMAND_START)) {
                optimized++;
                Logger::Log(L"Служба оптимизирована: " + service.name, LOG_SUCCESS);
            }
        }
    }
    
    Logger::Log(L"Оптимизировано служб: " + std::to_wstring(optimized), LOG_SUCCESS);
}

void ServicesModule::DisableTelemetry() {
    Logger::Log(L"Отключение телеметрии", LOG_INFO);
    
    // Отключение служб телеметрии
    const wchar_t* telemetryServices[] = {
        L"DiagTrack", L"dmwappushservice", L"WMPNetworkSvc", L"WpnService"
    };
    
    for (const wchar_t* service : telemetryServices) {
        SetServiceStartType(service, SERVICE_DISABLED);
        StopService(service);
    }
    
    // Отключение через реестр
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", 
        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        
        DWORD value = 0;
        RegSetValueEx(hKey, L"AllowTelemetry", 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
        RegCloseKey(hKey);
    }
    
    Logger::Log(L"Телеметрия отключена", LOG_SUCCESS);
}