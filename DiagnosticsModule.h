#pragma once
#include "Main.h"

class DiagnosticsModule {
public:
    static SystemInfo GetSystemInfo();
    static bool RunSFC();
    static bool RunDISM();
    static bool RunDISMCommand(const std::wstring& command);
    static bool IsSSD(const std::wstring& drive);
    static int CheckDiskHealth(const std::wstring& drive);
    static int GetEventLogErrors();
    static void RunFullDiagnostics();
};
