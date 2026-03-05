#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <windows.h>

struct RegistryValue {
    std::wstring name;
    DWORD type;
    std::wstring stringValue;
    DWORD dwordValue;
    DWORD64 qwordValue;
    std::vector<BYTE> binaryData;
};

class RegistryUtils {
public:
    static bool DeleteRegistryKey(HKEY hRootKey, const std::wstring& subKey);
    static std::vector<RegistryValue> EnumRegistryValues(HKEY hKey);
    static bool SetRegistryValue(HKEY hRootKey, const std::wstring& subKey,
                                  const std::wstring& valueName, DWORD type,
                                  const BYTE* data, DWORD dataSize);
    static std::wstring GetStringValue(HKEY hRootKey, const std::wstring& subKey,
                                        const std::wstring& valueName);
    static DWORD GetDwordValue(HKEY hRootKey, const std::wstring& subKey,
                                    const std::wstring& valueName, DWORD defaultValue);
    static bool DeleteRegistryValue(HKEY hRootKey, const std::wstring& subKey,
                                     const std::wstring& valueName);
    static void ExportRegistryKey(HKEY hRootKey, const std::wstring& subKey,
                                   const std::wstring& filePath);
};
