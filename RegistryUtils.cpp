#include "RegistryUtils.h"
#include "Logger.h"

bool RegistryUtils::DeleteRegistryKey(HKEY hRootKey, const std::wstring& subKey) {
    Logger::Log(L"Удаление ключа реестра: " + subKey, LOG_INFO);
    
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey.c_str(), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
        // Рекурсивное удаление всех подключей
        wchar_t subKeyName[256];
        DWORD subKeySize = 256;
        DWORD index = 0;
        
        while (RegEnumKeyEx(hKey, index, subKeyName, &subKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            DeleteRegistryKey(hKey, subKeyName);
            index = 0; // Сбрасываем индекс после удаления
            subKeySize = 256;
        }
        
        RegCloseKey(hKey);
        
        LONG result = RegDeleteKey(hRootKey, subKey.c_str());
        return (result == ERROR_SUCCESS);
    }
    
    return false;
}

std::vector<RegistryValue> RegistryUtils::EnumRegistryValues(HKEY hKey) {
    std::vector<RegistryValue> values;
    
    DWORD valueCount = 0;
    DWORD maxValueNameLen = 0;
    DWORD maxValueDataLen = 0;
    
    RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
                    &valueCount, &maxValueNameLen, &maxValueDataLen, NULL, NULL);
    
    maxValueNameLen++;
    maxValueDataLen++;
    
    wchar_t* valueName = new wchar_t[maxValueNameLen];
    BYTE* valueData = new BYTE[maxValueDataLen];
    
    for (DWORD i = 0; i < valueCount; i++) {
        DWORD valueNameSize = maxValueNameLen;
        DWORD valueDataSize = maxValueDataLen;
        DWORD type;
        
        if (RegEnumValue(hKey, i, valueName, &valueNameSize, NULL, &type,
                         valueData, &valueDataSize) == ERROR_SUCCESS) {
            
            RegistryValue value;
            value.name = valueName;
            value.type = type;
            
            // Получение данных в зависимости от типа
            switch (type) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    value.stringValue = (wchar_t*)valueData;
                    break;
                    
                case REG_DWORD:
                    if (valueDataSize == sizeof(DWORD)) {
                        value.dwordValue = *(DWORD*)valueData;
                    }
                    break;
                    
                case REG_QWORD:
                    if (valueDataSize == sizeof(DWORD64)) {
                        value.qwordValue = *(DWORD64*)valueData;
                    }
                    break;
                    
                case REG_BINARY:
                    value.binaryData.assign(valueData, valueData + valueDataSize);
                    break;
            }
            
            values.push_back(value);
        }
    }
    
    delete[] valueName;
    delete[] valueData;
    
    return values;
}

bool RegistryUtils::SetRegistryValue(HKEY hRootKey, const std::wstring& subKey,
                                      const std::wstring& valueName, DWORD type,
                                      const BYTE* data, DWORD dataSize) {
    HKEY hKey;
    DWORD disposition;
    
    LONG result = RegCreateKeyEx(hRootKey, subKey.c_str(), 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                  NULL, &hKey, &disposition);
    
    if (result == ERROR_SUCCESS) {
        result = RegSetValueEx(hKey, valueName.c_str(), 0, type, data, dataSize);
        RegCloseKey(hKey);
    }
    
    return (result == ERROR_SUCCESS);
}

std::wstring RegistryUtils::GetStringValue(HKEY hRootKey, const std::wstring& subKey,
                                            const std::wstring& valueName) {
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return L"";
    }
    
    wchar_t buffer[1024] = {0};
    DWORD size = sizeof(buffer);
    DWORD type;
    
    LONG result = RegQueryValueEx(hKey, valueName.c_str(), NULL, &type,
                                   (LPBYTE)buffer, &size);
    
    RegCloseKey(hKey);
    
    if (result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
        return buffer;
    }
    
    return L"";
}

DWORD RegistryUtils::GetDwordValue(HKEY hRootKey, const std::wstring& subKey,
                                    const std::wstring& valueName, DWORD defaultValue) {
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return defaultValue;
    }
    
    DWORD value;
    DWORD size = sizeof(value);
    DWORD type;
    
    LONG result = RegQueryValueEx(hKey, valueName.c_str(), NULL, &type,
                                   (LPBYTE)&value, &size);
    
    RegCloseKey(hKey);
    
    if (result == ERROR_SUCCESS && type == REG_DWORD) {
        return value;
    }
    
    return defaultValue;
}

bool RegistryUtils::DeleteRegistryValue(HKEY hRootKey, const std::wstring& subKey,
                                         const std::wstring& valueName) {
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    
    LONG result = RegDeleteValue(hKey, valueName.c_str());
    RegCloseKey(hKey);
    
    return (result == ERROR_SUCCESS);
}

void RegistryUtils::ExportRegistryKey(HKEY hRootKey, const std::wstring& subKey,
                                       const std::wstring& filePath) {
    std::wofstream file(filePath);
    
    if (!file.is_open()) return;
    
    file << L"Windows Registry Editor Version 5.00\r\n\r\n";
    
    // Заголовок ключа
    std::wstring rootStr;
    if (hRootKey == HKEY_CLASSES_ROOT) rootStr = L"HKEY_CLASSES_ROOT";
    else if (hRootKey == HKEY_CURRENT_USER) rootStr = L"HKEY_CURRENT_USER";
    else if (hRootKey == HKEY_LOCAL_MACHINE) rootStr = L"HKEY_LOCAL_MACHINE";
    else if (hRootKey == HKEY_USERS) rootStr = L"HKEY_USERS";
    else rootStr = L"HKLM";
    
    file << L"[" << rootStr << L"\\" << subKey << L"]\r\n";
    
    // Экспорт значений
    HKEY hKey;
    if (RegOpenKeyEx(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        auto values = EnumRegistryValues(hKey);
        
        for (const auto& value : values) {
            file << L"\"" << value.name << L"\"=";
            
            switch (value.type) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    file << L"\"" << value.stringValue << L"\"";
                    break;
                    
                case REG_DWORD:
                    file << L"dword:" << std::hex << std::setw(8) << std::setfill(L'0') 
                         << value.dwordValue;
                    break;
                    
                case REG_QWORD:
                    file << L"hex(b):" << std::hex << value.qwordValue;
                    break;
                    
                case REG_BINARY:
                    file << L"hex:";
                    for (size_t i = 0; i < value.binaryData.size(); i++) {
                        if (i > 0) file << L",";
                        file << std::hex << std::setw(2) << std::setfill(L'0') 
                             << (int)value.binaryData[i];
                    }
                    break;
            }
            
            file << L"\r\n";
        }
        
        RegCloseKey(hKey);
    }
    
    file.close();
}