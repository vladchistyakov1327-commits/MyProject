#include "DiagnosticsModule.h"
#include "Logger.h"

SystemInfo DiagnosticsModule::GetSystemInfo() {
    SystemInfo info = {0};
    
    // Информация об ОС
    OSVERSIONINFOEXW osvi = { sizeof(osvi) };
    #pragma warning(push)
    #pragma warning(disable: 4996)
    GetVersionExW((LPOSVERSIONINFOW)&osvi);
    #pragma warning(pop)
    
    if (IsWindows10OrGreater()) {
        info.osVersion = L"Windows 10/11";
        
        // Получаем точную версию через реестр
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            
            wchar_t productName[256];
            wchar_t releaseId[64];
            wchar_t currentBuild[64];
            DWORD size = sizeof(productName);
            
            if (RegQueryValueEx(hKey, L"ProductName", NULL, NULL, 
                               (LPBYTE)productName, &size) == ERROR_SUCCESS) {
                info.osVersion = productName;
            }
            
            size = sizeof(releaseId);
            if (RegQueryValueEx(hKey, L"ReleaseId", NULL, NULL, 
                               (LPBYTE)releaseId, &size) == ERROR_SUCCESS) {
                info.osVersion += L" версия " + std::wstring(releaseId);
            }
            
            size = sizeof(currentBuild);
            if (RegQueryValueEx(hKey, L"CurrentBuild", NULL, NULL, 
                               (LPBYTE)currentBuild, &size) == ERROR_SUCCESS) {
                info.osBuild = currentBuild;
            }
            
            RegCloseKey(hKey);
        }
    } else {
        info.osVersion = L"Windows " + std::to_wstring(osvi.dwMajorVersion) + 
                        L"." + std::to_wstring(osvi.dwMinorVersion);
    }
    
    // Информация о процессоре
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        wchar_t processorName[256];
        DWORD size = sizeof(processorName);
        if (RegQueryValueEx(hKey, L"ProcessorNameString", NULL, NULL, 
                            (LPBYTE)processorName, &size) == ERROR_SUCCESS) {
            info.processor = processorName;
            
            // Очистка лишних пробелов
            while (!info.processor.empty() && info.processor.back() == L' ') {
                info.processor.pop_back();
            }
        }
        RegCloseKey(hKey);
    }
    
    // Информация об ОЗУ
    MEMORYSTATUSEX memStatus = { sizeof(memStatus) };
    GlobalMemoryStatusEx(&memStatus);
    info.totalRAM = memStatus.ullTotalPhys;
    info.availableRAM = memStatus.ullAvailPhys;
    
    // Информация о диске
    GetDiskFreeSpaceEx(L"C:\\", NULL, &info.totalDiskSpace, &info.freeDiskSpace);
    
    // Тип диска
    info.isSSD = IsSSD(L"C:");
    
    // Количество ошибок в журнале
    info.errorCount = GetEventLogErrors();
    
    return info;
}

bool DiagnosticsModule::RunSFC() {
    Logger::Log(L"Запуск SFC /scannow", LOG_INFO);
    
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    if (CreateProcessW(NULL, (LPWSTR)L"sfc /scannow", NULL, NULL, TRUE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        
        // Чтение вывода
        char buffer[4096];
        DWORD bytesRead;
        std::string result;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        
        CloseHandle(hReadPipe);
        
        // Проверка результата
        if (result.find("Windows Resource Protection found corrupt files") != std::string::npos) {
            Logger::Log(L"SFC: Найдены поврежденные файлы", LOG_WARNING);
            return false;
        } else if (result.find("Windows Resource Protection did not find any integrity violations") != std::string::npos) {
            Logger::Log(L"SFC: Нарушений целостности не найдено", LOG_SUCCESS);
            return true;
        }
        
        return exitCode == 0;
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return false;
}

bool DiagnosticsModule::RunDISM() {
    Logger::Log(L"Запуск DISM /CheckHealth", LOG_INFO);
    
    // Сначала проверка здоровья
    if (!RunDISMCommand(L"DISM /Online /Cleanup-Image /CheckHealth")) {
        // Если проблемы, запускаем сканирование
        Logger::Log(L"Запуск DISM /ScanHealth", LOG_WARNING);
        if (!RunDISMCommand(L"DISM /Online /Cleanup-Image /ScanHealth")) {
            // Если серьезные проблемы, запускаем восстановление
            Logger::Log(L"Запуск DISM /RestoreHealth", LOG_WARNING);
            return RunDISMCommand(L"DISM /Online /Cleanup-Image /RestoreHealth");
        }
    }
    
    return true;
}

bool DiagnosticsModule::RunDISMCommand(const std::wstring& command) {
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        
        return exitCode == 0;
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return false;
}

bool DiagnosticsModule::IsSSD(const std::wstring& drive) {
    std::wstring path = drive + L"\\";
    std::wstring command = L"fsutil fsinfo sectorinfo " + path;
    
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        char buffer[4096];
        DWORD bytesRead;
        std::string result;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        
        // Поиск признаков SSD в выводе
        return result.find("SSD") != std::string::npos || 
               result.find("NonRotationalMedia") != std::string::npos ||
               result.find("NonRotational") != std::string::npos;
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return false;
}

int DiagnosticsModule::CheckDiskHealth(const std::wstring& drive) {
    // Используем WMI для получения статуса диска
    std::wstring command = L"wmic diskdrive where \"MediaType='Fixed hard disk media'\" get Status,Model";
    
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        char buffer[4096];
        DWORD bytesRead;
        std::string result;
        
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        
        // Анализ результата
        if (result.find("OK") != std::string::npos || result.find("Ok") != std::string::npos) {
            return 100; // Отлично
        } else if (result.find("Degraded") != std::string::npos || 
                   result.find("Pred Fail") != std::string::npos) {
            return 30; // Проблемы
        } else if (result.find("Unknown") != std::string::npos) {
            return 50; // Неизвестно
        }
        
        return 80; // Предположительно хорошо
    }
    
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return 0;
}

int DiagnosticsModule::GetEventLogErrors() {
    // Подсчет ошибок в системном журнале за последние 24 часа
    HANDLE hEventLog = OpenEventLog(NULL, L"System");
    if (!hEventLog) return 0;
    
    int errorCount = 0;
    BYTE buffer[1024];
    DWORD bytesRead, bytesNeeded;
    EVENTLOGRECORD* record;
    
    while (ReadEventLog(hEventLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                        0, buffer, sizeof(buffer), &bytesRead, &bytesNeeded)) {
        
        record = (EVENTLOGRECORD*)buffer;
        
        while (bytesRead > 0) {
            // Проверка типа события (1 = ошибка)
            if (record->EventType == EVENTLOG_ERROR_TYPE || 
                record->EventType == EVENTLOG_WARNING_TYPE) {
                
                // Проверка, что событие за последние 24 часа
                SYSTEMTIME st;
                FileTimeToSystemTime((FILETIME*)&record->TimeGenerated, &st);
                
                FILETIME ft;
                SystemTimeToFileTime(&st, &ft);
                
                ULARGE_INTEGER ulEvent = { ft.dwLowDateTime, ft.dwHighDateTime };
                
                FILETIME nowFt;
                GetSystemTimeAsFileTime(&nowFt);
                ULARGE_INTEGER ulNow = { nowFt.dwLowDateTime, nowFt.dwHighDateTime };
                
                // 24 часа в 100-наносекундных интервалах = 24 * 60 * 60 * 10000000
                if (ulNow.QuadPart - ulEvent.QuadPart < 864000000000) {
                    errorCount++;
                }
            }
            
            bytesRead -= record->Length;
            record = (EVENTLOGRECORD*)((BYTE*)record + record->Length);
        }
    }
    
    CloseEventLog(hEventLog);
    return errorCount;
}