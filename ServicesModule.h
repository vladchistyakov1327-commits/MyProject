#pragma once
#include <string>
#include <vector>
#include "Main.h"

class ServicesModule {
public:
    static std::vector<ServiceInfo> GetServices();
    static bool CanOptimizeService(const ServiceInfo& service);
    static bool SetServiceStartType(const std::wstring& serviceName, DWORD startType);
    static bool ControlService(const std::wstring& serviceName, DWORD control);
    static bool StartService(const std::wstring& serviceName);
    static bool StopService(const std::wstring& serviceName);
    static void OptimizeServices();
    static void DisableTelemetry();
};
