#pragma once
#include <string>
#include <vector>

struct NetworkAdapter {
    std::wstring name;
    std::wstring description;
    std::wstring type;
    std::wstring macAddress;
    std::vector<std::wstring> ipAddresses;
    bool isDHCPEnabled;
};

class NetworkModule {
public:
    static bool ResetNetworkSettings();
    static bool FlushDNS();
    static bool RenewIP();
    static std::vector<NetworkAdapter> GetNetworkAdapters();
    static void DiagnoseNetwork();
    static bool CheckInternetConnection();
    static bool CheckDNSResolution();
    static bool FindVPNConflicts();
};
