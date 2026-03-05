#include "ProgramsModule.h"
#include "Logger.h"
#include <winreg.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

std::vector<ProgramInfo> ProgramsModule::GetInstalledPrograms() {
    std::vector<ProgramInfo> programs;
    
    const wchar_t* regPaths[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };
    
    for (const wchar_t* regPath : regPaths) {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            DWORD index = 0;
            wchar_t subKeyName[256];
            DWORD subKeySize = 256;
            
            while (RegEnumKeyEx(hKey, index, subKeyName, &subKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                
                HKEY hSubKey;
                if (RegOpenKeyEx(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                    
                    ProgramInfo prog = {0};
                    
                    // DisplayName
                    wchar_t displayName[256] = {0};
                    DWORD size = sizeof(displayName);
                    DWORD type;
                    if (RegQueryValueEx(hSubKey, L"DisplayName", NULL, &type, 
                                       (LPBYTE)displayName, &size) == ERROR_SUCCESS && type == REG_SZ) {
                        prog.name = displayName;
                        
                        // DisplayVersion
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"DisplayVersion", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.version = displayName;
                        }
                        
                        // Publisher
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"Publisher", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.publisher = displayName;
                        }
                        
                        // InstallDate
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"InstallDate", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.installDate = displayName;
                        }
                        
                        // InstallLocation
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"InstallLocation", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.installLocation = displayName;
                        }
                        
                        // UninstallString
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"UninstallString", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.uninstallString = displayName;
                        }
                        
                        // QuietUninstallString
                        size = sizeof(displayName);
                        if (RegQueryValueEx(hSubKey, L"QuietUninstallString", NULL, NULL, 
                                           (LPBYTE)displayName, &size) == ERROR_SUCCESS) {
                            prog.quietUninstallString = displayName;
                        }
                        
                        // EstimatedSize
                        DWORD estSize;
                        size = sizeof(estSize);
                        if (RegQueryValueEx(hSubKey, L"EstimatedSize", NULL, NULL, 
                                           (LPBYTE)&estSize, &size) == ERROR_SUCCESS) {
                            prog.size = estSize * 1024LL; // KB to bytes
                        }
                        
                        // SystemComponent
                        DWORD sysComp;
                        size = sizeof(sysComp);
                        if (RegQueryValueEx(hSubKey, L"SystemComponent", NULL, NULL,
                                           (LPBYTE)&sysComp, &size) == ERROR_SUCCESS) {
                            prog.isSystem = (sysComp == 1);
                        }
                        
                        // WindowsInstaller
                        DWORD winInstaller;
                        size = sizeof(winInstaller);
                        if (RegQueryValueEx(hSubKey, L"WindowsInstaller", NULL, NULL,
                                           (LPBYTE)&winInstaller, &size) == ERROR_SUCCESS) {
                            prog.isWindowsInstaller = (winInstaller == 1);
                        }
                        
                        // Проверка на системное ПО по имени
                        if (!prog.isSystem) {
                            std::wstring nameLower = prog.name;
                            for (auto& c : nameLower) c = towlower(c);
                            
                            if (nameLower.find(L"microsoft") != std::wstring::npos ||
                                nameLower.find(L"windows") != std::wstring::npos ||
                                nameLower.find(L"visual c++") != std::wstring::npos ||
                                nameLower.find(L".net") != std::wstring::npos) {
                                prog.isSystem = true;
                            }
                        }
                        
                        // Проверка на конфликтующее ПО
                        prog.isConflict = IsConflictProgram(prog.name);
                        
                        // Получение даты последнего использования
                        if (!prog.installLocation.empty() && PathFileExists(prog.installLocation.c_str())) {
                            WIN32_FIND_DATA findData;
                            HANDLE hFind = FindFirstFile((prog.installLocation + L"\\*").c_str(), &findData);
                            if (hFind != INVALID_HANDLE_VALUE) {
                                FindClose(hFind);
                                prog.lastUsed = findData.ftLastWriteTime;
                            }
                        }
                        
                        programs.push_back(prog);
                    }
                    
                    RegCloseKey(hSubKey);
                }
                
                index++;
                subKeySize = 256;
            }
            
            RegCloseKey(hKey);
        }
    }
    
    // Сортировка по имени
    std::sort(programs.begin(), programs.end(), 
              [](const ProgramInfo& a, const ProgramInfo& b) {
                  return a.name < b.name;
              });
    
    return programs;
}

bool ProgramsModule::IsConflictProgram(const std::wstring& programName) {
    // Список конфликтующего ПО
    const wchar_t* conflictList[] = {
        L"VPN", L"NetBalancer", L"Java", L"Adobe AIR", L"Bonjour",
        L"iTunes", L"QuickTime", L"Flash", L"Shockwave",
        L"Yandex Browser", L"Mail.ru", L"Amigo", L"Comet", L"Baidu",
        L"CCleaner", L"Advanced SystemCare", L"Driver Booster",
        L"IObit", L"Glary", L"Wise Care", L"360 Total Security",
        L"Avast", L"AVG", L"McAfee", L"Norton", L"Kaspersky",
        L"TeamViewer", L"AnyDesk", L"Ammyy Admin",
        L"uTorrent", L"BitTorrent", L"MediaGet", L"Zona",
        L"Adobe Flash Player", L"Shockwave Player",
        L"Java 6", L"Java 7", L"Java 8 Update 20", L"Java 8 Update 25",
        L".NET Framework 1.0", L".NET Framework 1.1", L".NET Framework 2.0",
        L".NET Framework 3.0", L".NET Framework 3.5", L".NET Framework 4.0"
    };
    
    std::wstring nameLower = programName;
    for (auto& c : nameLower) c = towlower(c);
    
    for (const wchar_t* conflict : conflictList) {
        std::wstring conflictLower = conflict;
        for (auto& c : conflictLower) c = towlower(c);
        
        if (nameLower.find(conflictLower) != std::wstring::npos) {
            return true;
        }
    }
    
    return false;
}

bool ProgramsModule::UninstallProgram(const ProgramInfo& program) {
    if (program.uninstallString.empty() && program.quietUninstallString.empty()) {
        Logger::Log(L"Нет строки удаления для: " + program.name, LOG_WARNING);
        return false;
    }
    
    Logger::Log(L"Удаление программы: " + program.name, LOG_INFO);
    
    std::wstring command;
    
    // Используем тихое удаление если доступно
    if (!program.quietUninstallString.empty()) {
        command = program.quietUninstallString;
    } else {
        command = program.uninstallString;
        
        // Добавление тихих параметров в зависимости от типа установщика
        if (program.isWindowsInstaller) {
            // MSI installer
            command += L" /quiet /norestart /qn";
        } else {
            // InnoSetup, NSIS и другие
            if (command.find(L"uninst") != std::wstring::npos) {
                command += L" /S /silent /verysilent /suppressmsgboxes";
            }
        }
    }
    
    // Создание процесса удаления
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi;
    
    bool success = false;
    
    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, FALSE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        // Ждем завершения (максимум 60 секунд)
        WaitForSingleObject(pi.hProcess, 60000);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        success = (exitCode == 0);
        
        if (success) {
            Logger::Log(L"Программа успешно удалена: " + program.name, LOG_SUCCESS);
        } else {
            Logger::Log(L"Ошибка удаления (код " + std::to_wstring(exitCode) + 
                       L"): " + program.name, LOG_ERROR);
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        Logger::Log(L"Не удалось запустить процесс удаления для: " + program.name, LOG_ERROR);
    }
    
    return success;
}

void ProgramsModule::AnalyzeConflicts() {
    Logger::Log(L"Анализ конфликтов программ", LOG_INFO);
    
    auto programs = GetInstalledPrograms();
    int conflictCount = 0;
    
    for (const auto& prog : programs) {
        if (prog.isConflict) {
            conflictCount++;
            Logger::Log(L"Найдено конфликтующее ПО: " + prog.name, LOG_WARNING);
        }
    }
    
    Logger::Log(L"Всего конфликтующих программ: " + std::to_wstring(conflictCount), 
                conflictCount > 0 ? LOG_WARNING : LOG_SUCCESS);
}

std::vector<ProgramInfo> ProgramsModule::FindDuplicates() {
    std::vector<ProgramInfo> duplicates;
    auto programs = GetInstalledPrograms();
    
    for (size_t i = 0; i < programs.size(); i++) {
        for (size_t j = i + 1; j < programs.size(); j++) {
            // Проверка похожих имен
            if (programs[i].name.find(programs[j].name) != std::wstring::npos ||
                programs[j].name.find(programs[i].name) != std::wstring::npos) {
                
                // Проверка разных версий
                if (programs[i].version != programs[j].version) {
                    duplicates.push_back(programs[j]);
                }
            }
        }
    }
    
    return duplicates;
}