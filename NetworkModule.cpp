#include "NetworkModule.h"
#include <wininet.h>
#include "Logger.h"
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

bool NetworkModule::ResetNetworkSettings() {
    Logger::Log(L"РЎР±СЂРѕСЃ СЃРµС‚РµРІС‹С… РЅР°СЃС‚СЂРѕРµРє", LOG_INFO);
    
    // РЎР±СЂРѕСЃ Winsock
    _wsystem(L"netsh winsock reset");
    
    // РЎР±СЂРѕСЃ TCP/IP
    _wsystem(L"netsh int ip reset");
    
    // РЎР±СЂРѕСЃ Windows Firewall
    _wsystem(L"netsh advfirewall reset");
    
    // РЎР±СЂРѕСЃ РјР°СЂС€СЂСѓС‚РѕРІ
    _wsystem(L"route -f");
    
    Logger::Log(L"РЎРµС‚РµРІС‹Рµ РЅР°СЃС‚СЂРѕР№РєРё СЃР±СЂРѕС€РµРЅС‹", LOG_SUCCESS);
    
    return true;
}

bool NetworkModule::FlushDNS() {
    Logger::Log(L"РћС‡РёСЃС‚РєР° DNS", LOG_INFO);
    
    _wsystem(L"ipconfig /flushdns");
    
    Logger::Log(L"DNS РѕС‡РёС‰РµРЅ", LOG_SUCCESS);
    
    return true;
}

bool NetworkModule::RenewIP() {
    Logger::Log(L"РћР±РЅРѕРІР»РµРЅРёРµ IP Р°РґСЂРµСЃР°", LOG_INFO);
    
    _wsystem(L"ipconfig /release");
    _wsystem(L"ipconfig /renew");
    _wsystem(L"ipconfig /registerdns");
    
    Logger::Log(L"IP Р°РґСЂРµСЃ РѕР±РЅРѕРІР»РµРЅ", LOG_SUCCESS);
    
    return true;
}

std::vector<NetworkAdapter> NetworkModule::GetNetworkAdapters() {
    std::vector<NetworkAdapter> adapters;
    
    DWORD size = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    
    PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(size);
    
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &size) == NO_ERROR) {
        PIP_ADAPTER_ADDRESSES pCurr = pAddresses;
        
        while (pCurr) {
            if (pCurr->OperStatus == IfOperStatusUp) {
                NetworkAdapter adapter;
                adapter.name = pCurr->FriendlyName;
                adapter.description = pCurr->Description;
                
                // РўРёРї РїРѕРґРєР»СЋС‡РµРЅРёСЏ
                if (pCurr->IfType == IF_TYPE_ETHERNET_CSMACD) {
                    adapter.type = L"РџСЂРѕРІРѕРґРЅРѕРµ";
                } else if (pCurr->IfType == IF_TYPE_IEEE80211) {
                    adapter.type = L"Wi-Fi";
                } else {
                    adapter.type = L"Р”СЂСѓРіРѕРµ";
                }
                
                // IP Р°РґСЂРµСЃР°
                PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress;
                while (pUnicast) {
                    sockaddr_in* pAddr = (sockaddr_in*)pUnicast->Address.lpSockaddr;
                    if (pAddr->sin_family == AF_INET) {
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &pAddr->sin_addr, ipStr, INET_ADDRSTRLEN);
                        adapter.ipAddresses.push_back(std::wstring(ipStr, ipStr + strlen(ipStr)));
                    }
                    pUnicast = pUnicast->Next;
                }
                
                // MAC Р°РґСЂРµСЃ
                if (pCurr->PhysicalAddressLength > 0) {
                    wchar_t macStr[18];
                    swprintf(macStr, 18, L"%02X:%02X:%02X:%02X:%02X:%02X",
                             pCurr->PhysicalAddress[0], pCurr->PhysicalAddress[1],
                             pCurr->PhysicalAddress[2], pCurr->PhysicalAddress[3],
                             pCurr->PhysicalAddress[4], pCurr->PhysicalAddress[5]);
                    adapter.macAddress = macStr;
                }
                
                // РЎС‚Р°С‚СѓСЃ DHCP
                adapter.isDHCPEnabled = (pCurr->Dhcpv4Enabled != 0);
                
                adapters.push_back(adapter);
            }
            
            pCurr = pCurr->Next;
        }
    }
    
    free(pAddresses);
    return adapters;
}

void NetworkModule::DiagnoseNetwork() {
    Logger::Log(L"Р”РёР°РіРЅРѕСЃС‚РёРєР° СЃРµС‚Рё", LOG_INFO);
    
    // РџСЂРѕРІРµСЂРєР° РїРѕРґРєР»СЋС‡РµРЅРёСЏ Рє РёРЅС‚РµСЂРЅРµС‚Сѓ
    if (CheckInternetConnection()) {
        Logger::Log(L"РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє РёРЅС‚РµСЂРЅРµС‚Сѓ: Р•РЎРўР¬", LOG_SUCCESS);
    } else {
        Logger::Log(L"РџРѕРґРєР»СЋС‡РµРЅРёРµ Рє РёРЅС‚РµСЂРЅРµС‚Сѓ: РќР•Рў", LOG_ERROR);
    }
    
    // РџРѕР»СѓС‡РµРЅРёРµ Р°РґР°РїС‚РµСЂРѕРІ
    auto adapters = GetNetworkAdapters();
    Logger::Log(L"РќР°Р№РґРµРЅРѕ Р°РєС‚РёРІРЅС‹С… Р°РґР°РїС‚РµСЂРѕРІ: " + std::to_wstring(adapters.size()), LOG_INFO);
    
    // РџСЂРѕРІРµСЂРєР° DNS
    if (CheckDNSResolution()) {
        Logger::Log(L"DNS СЂРµР·РѕР»РІРёРЅРі: Р РђР‘РћРўРђР•Рў", LOG_SUCCESS);
    } else {
        Logger::Log(L"DNS СЂРµР·РѕР»РІРёРЅРі: РќР• Р РђР‘РћРўРђР•Рў", LOG_ERROR);
    }
    
    // РџСЂРѕРІРµСЂРєР° РЅР°Р»РёС‡РёСЏ VPN
    bool vpnFound = false;
    for (const auto& adapter : adapters) {
        if (adapter.description.find(L"VPN") != std::wstring::npos ||
            adapter.description.find(L"TAP") != std::wstring::npos ||
            adapter.description.find(L"Virtual") != std::wstring::npos) {
            vpnFound = true;
            Logger::Log(L"РќР°Р№РґРµРЅ VPN Р°РґР°РїС‚РµСЂ: " + adapter.description, LOG_WARNING);
        }
    }
    
    if (vpnFound) {
        Logger::Log(L"РћР±РЅР°СЂСѓР¶РµРЅС‹ VPN Р°РґР°РїС‚РµСЂС‹ - РІРѕР·РјРѕР¶РЅС‹ РєРѕРЅС„Р»РёРєС‚С‹", LOG_WARNING);
    }
}

bool NetworkModule::CheckInternetConnection() {
    return (InternetCheckConnection(L"http://www.microsoft.com", FLAG_ICC_FORCE_CONNECTION, 0) ||
            InternetCheckConnection(L"http://www.google.com", FLAG_ICC_FORCE_CONNECTION, 0));
}

bool NetworkModule::CheckDNSResolution() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    struct hostent* host = gethostbyname("www.microsoft.com");
    
    WSACleanup();
    
    return host != NULL;
}

bool NetworkModule::FindVPNConflicts() {
    Logger::Log(L"РџРѕРёСЃРє РєРѕРЅС„Р»РёРєС‚РѕРІ VPN", LOG_INFO);
    
    auto adapters = GetNetworkAdapters();
    bool conflictFound = false;
    
    for (const auto& adapter : adapters) {
        if (adapter.description.find(L"VPN") != std::wstring::npos ||
            adapter.description.find(L"TAP") != std::wstring::npos) {
            
            // РџСЂРѕРІРµСЂРєР° РєРѕРЅС„Р»РёРєС‚РѕРІ СЃ РґСЂСѓРіРёРјРё Р°РґР°РїС‚РµСЂР°РјРё
            for (const auto& other : adapters) {
                if (&adapter != &other && other.type == L"РџСЂРѕРІРѕРґРЅРѕРµ") {
                    Logger::Log(L"РџРѕС‚РµРЅС†РёР°Р»СЊРЅС‹Р№ РєРѕРЅС„Р»РёРєС‚: VPN " + adapter.name + 
                               L" Рё " + other.name, LOG_WARNING);
                    conflictFound = true;
                }
            }
        }
    }
    
    return conflictFound;
}
