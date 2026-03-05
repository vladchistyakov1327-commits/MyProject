#pragma once
#include <string>
#include <vector>

struct ProcessInfo {
    std::wstring name;
    DWORD pid;
    DWORD parentPid;
    DWORD threadCount;
    DWORD runningTime;
    std::wstring executablePath;
};

class SecurityModule {
public:
    static bool CreateRestorePoint(const std::wstring& description);
    static std::vector<ProcessInfo> GetSuspiciousProcesses();
    static bool IsSuspiciousProcess(const ProcessInfo& proc);
    static bool VerifyDigitalSignature(const std::wstring& filePath);
    static void ScanWithDefender();
    static bool CheckSystemFilesIntegrity();
};
