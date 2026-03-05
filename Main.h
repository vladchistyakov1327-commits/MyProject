#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winreg.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <versionhelpers.h>
#include <winternl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "version.lib")

// РљРѕРЅСЃС‚Р°РЅС‚С‹
#define ID_RUN_DIAGNOSTICS 1001
#define ID_RUN_CLEANUP 1002
#define ID_RUN_OPTIMIZATION 1003
#define ID_RUN_ALL 1004
#define ID_CREATE_RESTORE_POINT 1005
#define ID_VIEW_REPORT 1006
#define ID_SETTINGS 1007
#define ID_EXIT 1008
#define ID_REFRESH 1009
#define ID_EXPORT_REPORT 1010
#define ID_SCHEDULE_CLEANUP 1011

// Р РµР¶РёРјС‹ СЂР°Р±РѕС‚С‹
enum OperationMode {
    MODE_DIAGNOSTIC_ONLY,
    MODE_NORMAL,
    MODE_EXPERT
};

// РЎС‚СЂСѓРєС‚СѓСЂС‹ РґР°РЅРЅС‹С…
struct SystemInfo {
    std::wstring osVersion;
    std::wstring osBuild;
    std::wstring processor;
    DWORD64 totalRAM;
    DWORD64 availableRAM;
    ULARGE_INTEGER totalDiskSpace;
    ULARGE_INTEGER freeDiskSpace;
    int diskHealth;
    bool isSSD;
    int errorCount;
};

struct ProgramInfo {
    std::wstring name;
    std::wstring version;
    std::wstring publisher;
    std::wstring installDate;
    std::wstring installLocation;
    std::wstring uninstallString;
    DWORD64 size;
    FILETIME lastUsed;
    bool isConflict;
    bool isSystem;
};

struct ServiceInfo {
    std::wstring name;
    std::wstring displayName;
    std::wstring status;
    std::wstring startType;
    std::wstring description;
    bool canOptimize;
};

// Р¦РІРµС‚Р° РґР»СЏ РёРЅС‚РµСЂС„РµР№СЃР°
#define COLOR_BG RGB(30, 30, 30)
#define COLOR_TEXT RGB(220, 220, 220)
#define COLOR_ACCENT RGB(0, 120, 215)
#define COLOR_SUCCESS RGB(40, 180, 40)
#define COLOR_WARNING RGB(255, 140, 0)
#define COLOR_ERROR RGB(220, 40, 40)
// Константы операций
#define OP_DIAGNOSTICS 1
#define OP_CLEANUP 2
#define OP_OPTIMIZATION 3
#define OP_ALL 4

// Прототипы функций
void ViewReport();
void ShowTabContent(int tabIndex);
