#include <windows.h>
#include "core/traffic_monitor.h"
#include "utils/logger.h"

// Use common controls v6 for modern look
#ifdef _MSC_VER
#pragma comment(linker, \
    "\"/manifestdependency:type='win32' " \
    "name='Microsoft.Windows.Common-Controls' " \
    "version='6.0.0.0' processorArchitecture='*' " \
    "publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")
#endif

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR lpCmdLine, int nShowCmd) {
    // Enable high-DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Initialize common controls (needed for visual styles)
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    // Check command line for /minimized flag
    bool start_minimized = (lpCmdLine && wcsstr(lpCmdLine, L"/minimized"));

    LOG_INFO("Traffic Monitor v3.0 starting");

    core::TrafficMonitor monitor;
    if (!monitor.initialize(hInst, start_minimized)) {
        MessageBoxW(nullptr,
            L"Failed to initialize Traffic Monitor.\n"
            L"Please check the log file in %APPDATA%\\TrafficMonitor\\traffic.log",
            L"Traffic Monitor — Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    return monitor.run();
}
