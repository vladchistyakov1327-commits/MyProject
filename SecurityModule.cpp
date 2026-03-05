#include "SecurityModule.h"
#include "Logger.h"
#include <tlhelp32.h>
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

bool SecurityModule::CreateRestorePoint(const std::wstring& description) {
    Logger::Log(L"Создание точки восстановления: " + description, LOG_INFO);
    
    // Включение системного восстановления если отключено
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore", 
        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        
        DWORD value = 1;
        RegSetValueEx(hKey, L"RPSessionInterval", 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
        RegCloseKey(hKey);
    }
    
    // Создание точки восстановления через WMI
    std::wstring command = L"powershell -Command \"Checkpoint-Computer -Description '" + 
                          description + L"' -RestorePointType MODIFY_SETTINGS\"";
    
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    bool success = false;
    
    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, TRUE, 
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        
        WaitForSingleObject(pi.hProcess, 60000); // Ждем до 60 секунд
        
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        success = (exitCode == 0);
        
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
    }
    
    if (success) {
        Logger::Log(L"Точка восстановления создана успешно", LOG_SUCCESS);
    } else {
        Logger::Log(L"Не удалось создать точку восстановления", LOG_ERROR);
    }
    
    return success;
}

std::vector<ProcessInfo> SecurityModule::GetSuspiciousProcesses() {
    std::vector<ProcessInfo> suspicious;
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return suspicious;
    
    PROCESSENTRY32 pe = { sizeof(pe) };
    
    if (Process32First(hSnapshot, &pe)) {
        do {
            ProcessInfo proc;
            proc.name = pe.szExeFile;
            proc.pid = pe.th32ProcessID;
            proc.parentPid = pe.th32ParentProcessID;
            proc.threadCount = pe.cntThreads;
            
            // Открываем процесс для получения дополнительной информации
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
            if (hProcess) {
                // Время работы
                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                    FILETIME now;
                    GetSystemTimeAsFileTime(&now);
                    
                    ULARGE_INTEGER ulCreate = { createTime.dwLowDateTime, createTime.dwHighDateTime };
                    ULARGE_INTEGER ulNow = { now.dwLowDateTime, now.dwHighDateTime };
                    
                    proc.runningTime = (ulNow.QuadPart - ulCreate.QuadPart) / 10000000; // секунды
                }
                
                // Использование CPU
                FILETIME ftSysIdle, ftSysKernel, ftSysUser;
                if (GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser)) {
                    // Расчет CPU usage
                }
                
                // Путь к исполняемому файлу
                wchar_t exePath[MAX_PATH];
                DWORD size = MAX_PATH;
                if (QueryFullProcessImageName(hProcess, 0, exePath, &size)) {
                    proc.executablePath = exePath;
                    
                    // Проверка подозрительных признаков
                    if (IsSuspiciousProcess(proc)) {
                        suspicious.push_back(proc);
                    }
                }
                
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    return suspicious;
}

bool SecurityModule::IsSuspiciousProcess(const ProcessInfo& proc) {
    std::wstring nameLower = proc.name;
    for (auto& c : nameLower) c = towlower(c);
    
    // Список подозрительных имен процессов
    const wchar_t* suspiciousNames[] = {
        L"miner", L"bitcoin", L"monero", L"eth", L"crypt",
        L"keylog", L"hook", L"inject", L"rat", L"backdoor",
        L"spy", L"steal", L"password", L"dump", L"lsass",
        L"cmd.exe", L"powershell.exe", L"wscript.exe", L"cscript.exe"
    };
    
    for (const wchar_t* name : suspiciousNames) {
        if (nameLower.find(name) != std::wstring::npos) {
            return true;
        }
    }
    
    // Проверка цифровой подписи (отсутствие подписи может быть подозрительным)
    if (!proc.executablePath.empty()) {
        return !VerifyDigitalSignature(proc.executablePath);
    }
    
    return false;
}

bool SecurityModule::VerifyDigitalSignature(const std::wstring& filePath) {
    WINTRUST_FILE_INFO fileInfo = {0};
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = filePath.c_str();
    
    GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    WINTRUST_DATA trustData = {0};
    trustData.cbStruct = sizeof(trustData);
    trustData.dwUIChoice = WTD_UI_NONE;
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice = WTD_CHOICE_FILE;
    trustData.pFile = &fileInfo;
    
    LONG status = WinVerifyTrust(NULL, &guidAction, &trustData);
    
    return status == ERROR_SUCCESS;
}

void SecurityModule::ScanWithDefender() {
    Logger::Log(L"Запуск сканирования Windows Defender", LOG_INFO);
    
    // Быстрое сканирование
    _wsystem(L"start \"\" \"C:\\Program Files\\Windows Defender\\MpCmdRun.exe\" -Scan -ScanType 1");
    
    Logger::Log(L"Сканирование Defender запущено", LOG_SUCCESS);
}

bool SecurityModule::CheckSystemFilesIntegrity() {
    Logger::Log(L"Проверка целостности системных файлов", LOG_INFO);
    
    // Запуск SFC
    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
    
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    
    PROCESS_INFORMATION pi;
    
    bool integrityOk = true;
    
    if (CreateProcessW(NULL, (LPWSTR)L"sfc /verifyonly", NULL, NULL, TRUE, 
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
        
        // Проверка результата
        if (result.find("did not find any integrity violations") == std::string::npos) {
            integrityOk = false;
            Logger::Log(L"Найдены нарушения целостности системных файлов", LOG_WARNING);
        } else {
            Logger::Log(L"Целостность системных файлов подтверждена", LOG_SUCCESS);
        }
    }
    
    return integrityOk;
}