#include "Main.h"
#include "AdminChecker.h"
#include "DiagnosticsModule.h"
#include "CleanerModule.h"
#include "ProgramsModule.h"
#include "ServicesModule.h"
#include "NetworkModule.h"
#include "SecurityModule.h"
#include "SchedulerModule.h"
#include "Logger.h"

// Р“Р»РѕР±Р°Р»СЊРЅС‹Рµ РїРµСЂРµРјРµРЅРЅС‹Рµ
HINSTANCE hInst;
HWND hMainWnd;
HWND hTabControl;
HWND hStatusBar;
HWND hLogList;
HWND hProgressBar;
HWND hBtnRunAll;
HWND hBtnDiagnostics;
HWND hBtnCleanup;
HWND hBtnOptimization;
HWND hBtnRestorePoint;
HWND hCurrentDisplay;
SystemInfo g_sysInfo;
std::vector<ProgramInfo> g_programs;
std::vector<ServiceInfo> g_services;
OperationMode g_mode = MODE_NORMAL;
bool g_isAdmin = false;
std::wstring g_logFile;

// РџСЂРѕС‚РѕС‚РёРїС‹ С„СѓРЅРєС†РёР№
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK SettingsDlgProc(HWND, UINT, WPARAM, LPARAM);
void InitializeControls(HWND hWnd);
void UpdateSystemInfo();
void UpdateRecommendations();
void AddLogMessage(const std::wstring& message, COLORREF color = COLOR_TEXT);
void RunOperation(int operationId);
void CreateRestorePoint();
void ExportReport();
void LoadPrograms();
void LoadServices();

// РўРѕС‡РєР° РІС…РѕРґР°
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // РџСЂРѕРІРµСЂРєР° РїСЂР°РІ Р°РґРјРёРЅРёСЃС‚СЂР°С‚РѕСЂР°
    g_isAdmin = IsRunningAsAdmin();
    if (!g_isAdmin) {
        RequestAdminRights();
        return 0;
    }
    
    // РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ
    hInst = hInstance;
    InitCommonControls();
    
    // РЎРѕР·РґР°РЅРёРµ РіР»Р°РІРЅРѕРіРѕ РѕРєРЅР°
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.lpszClassName = L"SystemCleanerPro";
    
    RegisterClassEx(&wc);
    
    hMainWnd = CreateWindowEx(
        0, L"SystemCleanerPro", L"System Cleaner Pro v1.0",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hMainWnd) return 0;
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ Р»РѕРіР°
    g_logFile = L"SystemCleanerLog_" + GetCurrentDateTime() + L".txt";
    Logger::Initialize(g_logFile);
    Logger::Log(L"РџСЂРѕРіСЂР°РјРјР° Р·Р°РїСѓС‰РµРЅР°", LOG_INFO);
    
    // Р—Р°РїСѓСЃРє РґРёР°РіРЅРѕСЃС‚РёРєРё РїСЂРё СЃС‚Р°СЂС‚Рµ
    std::thread(UpdateSystemInfo).detach();
    
    // РћСЃРЅРѕРІРЅРѕР№ С†РёРєР» СЃРѕРѕР±С‰РµРЅРёР№
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}

// РћРєРѕРЅРЅР°СЏ РїСЂРѕС†РµРґСѓСЂР°
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            InitializeControls(hWnd);
            break;
            
        case WM_SIZE:
            {
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                
                // РР·РјРµРЅРµРЅРёРµ СЂР°Р·РјРµСЂРѕРІ СЌР»РµРјРµРЅС‚РѕРІ
                if (hTabControl) {
                    SetWindowPos(hTabControl, NULL, 10, 40, 
                                 rcClient.right - 20, rcClient.bottom - 120, SWP_NOZORDER);
                }
                
                if (hStatusBar) {
                    SendMessage(hStatusBar, WM_SIZE, 0, 0);
                }
                
                if (hLogList) {
                    SetWindowPos(hLogList, NULL, 10, rcClient.bottom - 110,
                                 rcClient.right - 20, 80, SWP_NOZORDER);
                }
                
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_RUN_DIAGNOSTICS:
                    RunOperation(OP_DIAGNOSTICS);
                    break;
                    
                case ID_RUN_CLEANUP:
                    RunOperation(OP_CLEANUP);
                    break;
                    
                case ID_RUN_OPTIMIZATION:
                    RunOperation(OP_OPTIMIZATION);
                    break;
                    
                case ID_RUN_ALL:
                    RunOperation(OP_ALL);
                    break;
                    
                case ID_CREATE_RESTORE_POINT:
                    CreateRestorePoint();
                    break;
                    
                case ID_VIEW_REPORT:
                    ViewReport();
                    break;
                    
                case ID_SETTINGS:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDlgProc);
                    break;
                    
                case ID_EXPORT_REPORT:
                    ExportReport();
                    break;
                    
                case ID_REFRESH:
                    UpdateSystemInfo();
                    break;
                    
                case ID_EXIT:
                    DestroyWindow(hWnd);
                    break;
            }
            break;
            
        case WM_NOTIFY:
            {
                NMHDR* pNMHDR = (NMHDR*)lParam;
                if (pNMHDR->hwndFrom == hTabControl && pNMHDR->code == TCN_SELCHANGE) {
                    // РЎРјРµРЅР° РІРєР»Р°РґРєРё
                    int sel = TabCtrl_GetCurSel(hTabControl);
                    ShowTabContent(sel);
                }
            }
            break;
            
        case WM_CTLCOLORSTATIC:
            {
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, COLOR_TEXT);
                SetBkColor(hdcStatic, COLOR_BG);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;
            
        case WM_DESTROY:
            Logger::Log(L"РџСЂРѕРіСЂР°РјРјР° Р·Р°РІРµСЂС€РµРЅР°", LOG_INFO);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// РРЅРёС†РёР°Р»РёР·Р°С†РёСЏ СЌР»РµРјРµРЅС‚РѕРІ СѓРїСЂР°РІР»РµРЅРёСЏ
void InitializeControls(HWND hWnd) {
    // РЎРѕР·РґР°РЅРёРµ РІРєР»Р°РґРѕРє
    hTabControl = CreateWindow(WC_TABCONTROL, NULL, 
                               WS_CHILD | WS_VISIBLE | TCS_FIXEDWIDTH,
                               10, 40, 860, 500, hWnd, NULL, hInst, NULL);
    
    TCITEM tie = {0};
    tie.mask = TCIF_TEXT;
    
    std::wstring tabs[] = {
        L"Р“Р»Р°РІРЅР°СЏ", L"Р”РёР°РіРЅРѕСЃС‚РёРєР°", L"РћС‡РёСЃС‚РєР°", 
        L"РџСЂРѕРіСЂР°РјРјС‹", L"РЎР»СѓР¶Р±С‹", L"РЎРµС‚СЊ", L"Р‘РµР·РѕРїР°СЃРЅРѕСЃС‚СЊ", L"РџР»Р°РЅРёСЂРѕРІС‰РёРє"
    };
    
    for (int i = 0; i < 8; i++) {
        tie.pszText = (LPWSTR)tabs[i].c_str();
        TabCtrl_InsertItem(hTabControl, i, &tie);
    }
    
    // РЎРѕР·РґР°РЅРёРµ РєРЅРѕРїРѕРє
    hBtnDiagnostics = CreateWindow(L"BUTTON", L"Р”РёР°РіРЅРѕСЃС‚РёРєР°",
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   10, 550, 100, 30, hWnd, (HMENU)ID_RUN_DIAGNOSTICS, hInst, NULL);
    
    hBtnCleanup = CreateWindow(L"BUTTON", L"РћС‡РёСЃС‚РєР°",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               120, 550, 100, 30, hWnd, (HMENU)ID_RUN_CLEANUP, hInst, NULL);
    
    hBtnOptimization = CreateWindow(L"BUTTON", L"РћРїС‚РёРјРёР·Р°С†РёСЏ",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     230, 550, 100, 30, hWnd, (HMENU)ID_RUN_OPTIMIZATION, hInst, NULL);
    
    hBtnRunAll = CreateWindow(L"BUTTON", L"Р’С‹РїРѕР»РЅРёС‚СЊ РІСЃС‘",
                               WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                               340, 550, 120, 30, hWnd, (HMENU)ID_RUN_ALL, hInst, NULL);
    
    hBtnRestorePoint = CreateWindow(L"BUTTON", L"РўРѕС‡РєР° РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ",
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     470, 550, 140, 30, hWnd, (HMENU)ID_CREATE_RESTORE_POINT, hInst, NULL);
    
    // РЎРѕР·РґР°РЅРёРµ СЃРїРёСЃРєР° Р»РѕРіР°
    hLogList = CreateWindow(L"LISTBOX", NULL,
                            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
                            10, 590, 860, 80, hWnd, NULL, hInst, NULL);
    
    // РЎРѕР·РґР°РЅРёРµ СЃС‚СЂРѕРєРё СЃРѕСЃС‚РѕСЏРЅРёСЏ
    hStatusBar = CreateWindow(STATUSCLASSNAME, NULL,
                              WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                              0, 0, 0, 0, hWnd, NULL, hInst, NULL);
    
    // РЎРѕР·РґР°РЅРёРµ РїСЂРѕРіСЂРµСЃСЃ-Р±Р°СЂР°
    hProgressBar = CreateWindow(PROGRESS_CLASS, NULL,
                                 WS_CHILD | WS_VISIBLE,
                                 620, 550, 250, 30, hWnd, NULL, hInst, NULL);
    SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hProgressBar, PBM_SETSTEP, 1, 0);
    
    // РЈСЃС‚Р°РЅРѕРІРєР° С€СЂРёС„С‚РѕРІ
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    SendMessage(hBtnDiagnostics, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnCleanup, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnOptimization, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnRunAll, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hBtnRestorePoint, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// РћР±РЅРѕРІР»РµРЅРёРµ РёРЅС„РѕСЂРјР°С†РёРё Рѕ СЃРёСЃС‚РµРјРµ
void UpdateSystemInfo() {
    SendMessage(hProgressBar, PBM_SETPOS, 10, 0);
    AddLogMessage(L"РЎР±РѕСЂ РёРЅС„РѕСЂРјР°С†РёРё Рѕ СЃРёСЃС‚РµРјРµ...", COLOR_ACCENT);
    
    g_sysInfo = DiagnosticsModule::GetSystemInfo();
    
    SendMessage(hProgressBar, PBM_SETPOS, 30, 0);
    AddLogMessage(L"РџСЂРѕРІРµСЂРєР° СЃРёСЃС‚РµРјРЅС‹С… С„Р°Р№Р»РѕРІ...", COLOR_ACCENT);
    
    bool sfcResult = DiagnosticsModule::RunSFC();
    if (!sfcResult) {
        AddLogMessage(L"РќР°Р№РґРµРЅС‹ РїРѕРІСЂРµР¶РґРµРЅРёСЏ СЃРёСЃС‚РµРјРЅС‹С… С„Р°Р№Р»РѕРІ", COLOR_WARNING);
    }
    
    SendMessage(hProgressBar, PBM_SETPOS, 50, 0);
    AddLogMessage(L"РџСЂРѕРІРµСЂРєР° РѕР±СЂР°Р·Р° СЃРёСЃС‚РµРјС‹...", COLOR_ACCENT);
    
    bool dismResult = DiagnosticsModule::RunDISM();
    if (!dismResult) {
        AddLogMessage(L"РћР±СЂР°Р· СЃРёСЃС‚РµРјС‹ С‚СЂРµР±СѓРµС‚ РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ", COLOR_WARNING);
    }
    
    SendMessage(hProgressBar, PBM_SETPOS, 70, 0);
    AddLogMessage(L"РђРЅР°Р»РёР· РґРёСЃРєР°...", COLOR_ACCENT);
    
    g_sysInfo.isSSD = DiagnosticsModule::IsSSD(L"C:");
    g_sysInfo.diskHealth = DiagnosticsModule::CheckDiskHealth(L"C:");
    
    SendMessage(hProgressBar, PBM_SETPOS, 90, 0);
    AddLogMessage(L"Р—Р°РіСЂСѓР·РєР° СЃРїРёСЃРєР° РїСЂРѕРіСЂР°РјРј...", COLOR_ACCENT);
    
    LoadPrograms();
    LoadServices();
    
    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    AddLogMessage(L"Р”РёР°РіРЅРѕСЃС‚РёРєР° Р·Р°РІРµСЂС€РµРЅР°", COLOR_SUCCESS);
    
    UpdateRecommendations();
    InvalidateRect(hMainWnd, NULL, TRUE);
}

// РћР±РЅРѕРІР»РµРЅРёРµ СЂРµРєРѕРјРµРЅРґР°С†РёР№
void UpdateRecommendations() {
    std::wstringstream ss;
    ss << L"РЎРёСЃС‚РµРјР°: " << g_sysInfo.osVersion << L" | ";
    ss << L"Р”РёСЃРє C: " << FormatBytes(g_sysInfo.freeDiskSpace.QuadPart) 
       << L" СЃРІРѕР±РѕРґРЅРѕ РёР· " << FormatBytes(g_sysInfo.totalDiskSpace.QuadPart);
    
    SetWindowText(hStatusBar, ss.str().c_str());
}

// Р”РѕР±Р°РІР»РµРЅРёРµ СЃРѕРѕР±С‰РµРЅРёСЏ РІ Р»РѕРі
void AddLogMessage(const std::wstring& message, COLORREF color) {
    std::wstring timeStr = GetCurrentTimeString();
    std::wstring logEntry = L"[" + timeStr + L"] " + message;
    
    // Р”РѕР±Р°РІР»РµРЅРёРµ РІ СЃРїРёСЃРѕРє
    SendMessage(hLogList, LB_ADDSTRING, 0, (LPARAM)logEntry.c_str());
    SendMessage(hLogList, LB_SETTOPINDEX, SendMessage(hLogList, LB_GETCOUNT, 0, 0) - 1, 0);
    
    // Р—Р°РїРёСЃСЊ РІ С„Р°Р№Р»
    Logger::Log(message, LOG_INFO);
}

// Р—Р°РїСѓСЃРє РѕРїРµСЂР°С†РёР№
void RunOperation(int operationId) {
    std::thread([operationId]() {
        switch (operationId) {
            case OP_DIAGNOSTICS:
                DiagnosticsModule::RunFullDiagnostics();
                break;
                
            case OP_CLEANUP:
                {
                    AddLogMessage(L"РќР°С‡Р°Р»Рѕ РѕС‡РёСЃС‚РєРё СЃРёСЃС‚РµРјС‹...", COLOR_ACCENT);
                    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
                    
                    // РЎРѕР·РґР°РЅРёРµ С‚РѕС‡РєРё РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ
                    if (MessageBox(hMainWnd, L"РЎРѕР·РґР°С‚СЊ С‚РѕС‡РєСѓ РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ РїРµСЂРµРґ РѕС‡РёСЃС‚РєРѕР№?", 
                                   L"РџРѕРґС‚РІРµСЂР¶РґРµРЅРёРµ", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        CreateRestorePoint();
                    }
                    
                    // РћС‡РёСЃС‚РєР° РїРѕ С€Р°РіР°Рј
                    int step = 0;
                    int totalSteps = 12;
                    
                    auto updateProgress = [&]() {
                        step++;
                        int progress = (step * 100) / totalSteps;
                        SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
                    };
                    
                    // 1. Р’СЂРµРјРµРЅРЅС‹Рµ С„Р°Р№Р»С‹ Windows
                    CleanerModule::CleanTempFiles();
                    AddLogMessage(L"РћС‡РёС‰РµРЅС‹ РІСЂРµРјРµРЅРЅС‹Рµ С„Р°Р№Р»С‹ Windows", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 2. РљСЌС€ Р±СЂР°СѓР·РµСЂРѕРІ
                    CleanerModule::CleanBrowserCache(L"Chrome");
                    CleanerModule::CleanBrowserCache(L"Edge");
                    CleanerModule::CleanBrowserCache(L"Firefox");
                    AddLogMessage(L"РћС‡РёС‰РµРЅ РєСЌС€ Р±СЂР°СѓР·РµСЂРѕРІ", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 3. РЎРёСЃС‚РµРјРЅС‹Рµ Р»РѕРіРё
                    CleanerModule::CleanEventLogs();
                    AddLogMessage(L"РћС‡РёС‰РµРЅС‹ СЃРёСЃС‚РµРјРЅС‹Рµ Р»РѕРіРё", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 4. РљСЌС€ РѕР±РЅРѕРІР»РµРЅРёР№
                    CleanerModule::CleanWindowsUpdateCache();
                    AddLogMessage(L"РћС‡РёС‰РµРЅ РєСЌС€ РѕР±РЅРѕРІР»РµРЅРёР№ Windows", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 5. Prefetch
                    CleanerModule::CleanPrefetch();
                    AddLogMessage(L"РћС‡РёС‰РµРЅ Prefetch", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 6. DNS РєСЌС€
                    CleanerModule::FlushDNS();
                    AddLogMessage(L"РЎР±СЂРѕС€РµРЅ DNS РєСЌС€", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 7. РљРѕСЂР·РёРЅР°
                    CleanerModule::EmptyRecycleBin();
                    AddLogMessage(L"РћС‡РёС‰РµРЅР° РєРѕСЂР·РёРЅР°", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 8. Thumbnail cache
                    CleanerModule::CleanThumbnailCache();
                    AddLogMessage(L"РћС‡РёС‰РµРЅ РєСЌС€ РїСЂРµРІСЊСЋ", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 9. Р›РѕРіРё CBS/DISM
                    CleanerModule::CleanCBSLogs();
                    AddLogMessage(L"РћС‡РёС‰РµРЅС‹ Р»РѕРіРё CBS/DISM", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 10. РљСЌС€ РёРєРѕРЅРѕРє
                    CleanerModule::RebuildIconCache();
                    AddLogMessage(L"РџРµСЂРµСЃС‚СЂРѕРµРЅ РєСЌС€ РёРєРѕРЅРѕРє", COLOR_SUCCESS);
                    updateProgress();
                    
                    // 11. WinSxS
                    if (g_mode == MODE_EXPERT || 
                        MessageBox(hMainWnd, L"Р’С‹РїРѕР»РЅРёС‚СЊ РіР»СѓР±РѕРєСѓСЋ РѕС‡РёСЃС‚РєСѓ WinSxS?\n(РјРѕР¶РµС‚ Р·Р°РЅСЏС‚СЊ РјРЅРѕРіРѕ РІСЂРµРјРµРЅРё)", 
                                   L"РџРѕРґС‚РІРµСЂР¶РґРµРЅРёРµ", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        CleanerModule::CleanWinSxS();
                        AddLogMessage(L"Р’С‹РїРѕР»РЅРµРЅР° РѕС‡РёСЃС‚РєР° WinSxS", COLOR_SUCCESS);
                    }
                    updateProgress();
                    
                    // 12. Р’СЂРµРјРµРЅРЅС‹Рµ С„Р°Р№Р»С‹ РїСЂРѕРµРєС‚РѕРІ
                    CleanerModule::CleanProjectTempFiles();
                    AddLogMessage(L"РћС‡РёС‰РµРЅС‹ РІСЂРµРјРµРЅРЅС‹Рµ С„Р°Р№Р»С‹ РїСЂРѕРµРєС‚РѕРІ", COLOR_SUCCESS);
                    updateProgress();
                    
                    AddLogMessage(L"РћС‡РёСЃС‚РєР° СЃРёСЃС‚РµРјС‹ Р·Р°РІРµСЂС€РµРЅР°!", COLOR_SUCCESS);
                    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
                    
                    // РџСЂРµРґР»РѕР¶РµРЅРёРµ РїРµСЂРµР·Р°РіСЂСѓР·РєРё
                    if (MessageBox(hMainWnd, L"РћС‡РёСЃС‚РєР° Р·Р°РІРµСЂС€РµРЅР°. РџРµСЂРµР·Р°РіСЂСѓР·РёС‚СЊ РєРѕРјРїСЊСЋС‚РµСЂ СЃРµР№С‡Р°СЃ?", 
                                   L"РџРµСЂРµР·Р°РіСЂСѓР·РєР°", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        system("shutdown /r /t 10 /c \"System Cleaner Pro Р·Р°РІРµСЂС€РёР» РѕС‡РёСЃС‚РєСѓ. РџРµСЂРµР·Р°РіСЂСѓР·РєР° С‡РµСЂРµР· 10 СЃРµРєСѓРЅРґ...\"");
                    }
                }
                break;
                
            case OP_OPTIMIZATION:
                {
                    AddLogMessage(L"РќР°С‡Р°Р»Рѕ РѕРїС‚РёРјРёР·Р°С†РёРё СЃРёСЃС‚РµРјС‹...", COLOR_ACCENT);
                    
                    // 1. РћС‚РєР»СЋС‡РµРЅРёРµ РіРёР±РµСЂРЅР°С†РёРё
                    if (g_sysInfo.freeDiskSpace.QuadPart < 20LL * 1024 * 1024 * 1024) {
                        system("powercfg -h off");
                        AddLogMessage(L"Р“РёР±РµСЂРЅР°С†РёСЏ РѕС‚РєР»СЋС‡РµРЅР° (+РѕСЃРІРѕР±РѕР¶РґРµРЅРѕ ~8 Р“Р‘)", COLOR_SUCCESS);
                    }
                    
                    // 2. Р”РµС„СЂР°РіРјРµРЅС‚Р°С†РёСЏ/TRIM
                    if (g_sysInfo.isSSD) {
                        system("defrag C: /L /O");
                        AddLogMessage(L"Р’С‹РїРѕР»РЅРµРЅ TRIM РґР»СЏ SSD", COLOR_SUCCESS);
                    } else {
                        system("defrag C: /O");
                        AddLogMessage(L"Р’С‹РїРѕР»РЅРµРЅР° РґРµС„СЂР°РіРјРµРЅС‚Р°С†РёСЏ РґРёСЃРєР°", COLOR_SUCCESS);
                    }
                    
                    // 3. РќР°СЃС‚СЂРѕР№РєР° РїСЂРѕРёР·РІРѕРґРёС‚РµР»СЊРЅРѕСЃС‚Рё
                    system("fsutil behavior set disablelastaccess 1");
                    system("fsutil behavior set disable8dot3 1");
                    AddLogMessage(L"РћРїС‚РёРјРёР·Р°С†РёСЏ С„Р°Р№Р»РѕРІРѕР№ СЃРёСЃС‚РµРјС‹ РІС‹РїРѕР»РЅРµРЅР°", COLOR_SUCCESS);
                    
                    // 4. РћС‡РёСЃС‚РєР° СЃРёСЃС‚РµРјРЅРѕРіРѕ РєСЌС€Р°
                    system("rundll32.exe advapi32.dll,ProcessIdleTasks");
                    
                    AddLogMessage(L"РћРїС‚РёРјРёР·Р°С†РёСЏ Р·Р°РІРµСЂС€РµРЅР°!", COLOR_SUCCESS);
                }
                break;
                
            case OP_ALL:
                {
                    RunOperation(OP_DIAGNOSTICS);
                    RunOperation(OP_CLEANUP);
                    RunOperation(OP_OPTIMIZATION);
                    
                    // РђРЅР°Р»РёР· РїСЂРѕРіСЂР°РјРј
                    ProgramsModule::AnalyzeConflicts();
                    AddLogMessage(L"РђРЅР°Р»РёР· РїСЂРѕРіСЂР°РјРј Р·Р°РІРµСЂС€РµРЅ", COLOR_SUCCESS);
                    
                    // РЎРµС‚РµРІС‹Рµ РЅР°СЃС‚СЂРѕР№РєРё
                    NetworkModule::DiagnoseNetwork();
                    AddLogMessage(L"Р”РёР°РіРЅРѕСЃС‚РёРєР° СЃРµС‚Рё Р·Р°РІРµСЂС€РµРЅР°", COLOR_SUCCESS);
                    
                    MessageBox(hMainWnd, L"Р’СЃРµ РѕРїРµСЂР°С†РёРё СѓСЃРїРµС€РЅРѕ РІС‹РїРѕР»РЅРµРЅС‹!", 
                               L"Р“РѕС‚РѕРІРѕ", MB_OK | MB_ICONINFORMATION);
                }
                break;
        }
    }).detach();
}

// РЎРѕР·РґР°РЅРёРµ С‚РѕС‡РєРё РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ
void CreateRestorePoint() {
    AddLogMessage(L"РЎРѕР·РґР°РЅРёРµ С‚РѕС‡РєРё РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ...", COLOR_ACCENT);
    
    std::wstring command = L"powershell -Command \"Checkpoint-Computer -Description 'System Cleaner Pro Restore Point' -RestorePointType MODIFY_SETTINGS\"";
    
    if (SecurityModule::CreateRestorePoint(L"System Cleaner Pro")) {
        AddLogMessage(L"РўРѕС‡РєР° РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ СЃРѕР·РґР°РЅР° СѓСЃРїРµС€РЅРѕ", COLOR_SUCCESS);
    } else {
        AddLogMessage(L"РћС€РёР±РєР° СЃРѕР·РґР°РЅРёСЏ С‚РѕС‡РєРё РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ", COLOR_ERROR);
    }
}

// Р­РєСЃРїРѕСЂС‚ РѕС‚С‡РµС‚Р°
void ExportReport() {
    OPENFILENAME ofn = {0};
    wchar_t fileName[MAX_PATH] = L"SystemReport.html";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"HTML Files\0*.html\0Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileName(&ofn)) {
        std::wofstream file(fileName);
        
        file << L"<html><head><title>System Report</title>";
        file << L"<style>body{font-family:Arial;margin:20px;}";
        file << L"h1{color:#0078D7;} table{border-collapse:collapse;width:100%;} ";
        file << L"th,td{border:1px solid #ddd;padding:8px;text-align:left;} ";
        file << L"th{background-color:#0078D7;color:white;}</style></head><body>";
        
        file << L"<h1>System Cleaner Pro - РћС‚С‡РµС‚ РґРёР°РіРЅРѕСЃС‚РёРєРё</h1>";
        file << L"<p>Р”Р°С‚Р°: " << GetCurrentDateTime() << L"</p>";
        
        // РРЅС„РѕСЂРјР°С†РёСЏ Рѕ СЃРёСЃС‚РµРјРµ
        file << L"<h2>РРЅС„РѕСЂРјР°С†РёСЏ Рѕ СЃРёСЃС‚РµРјРµ</h2>";
        file << L"<table>";
        file << L"<tr><th>РџР°СЂР°РјРµС‚СЂ</th><th>Р—РЅР°С‡РµРЅРёРµ</th></tr>";
        file << L"<tr><td>РћРЎ</td><td>" << g_sysInfo.osVersion << L"</td></tr>";
        file << L"<tr><td>РџСЂРѕС†РµСЃСЃРѕСЂ</td><td>" << g_sysInfo.processor << L"</td></tr>";
        file << L"<tr><td>РћР—РЈ</td><td>" << FormatBytes(g_sysInfo.totalRAM) << L" (РґРѕСЃС‚СѓРїРЅРѕ " 
             << FormatBytes(g_sysInfo.availableRAM) << L")</td></tr>";
        file << L"<tr><td>Р”РёСЃРє C:</td><td>" << FormatBytes(g_sysInfo.freeDiskSpace.QuadPart) 
             << L" СЃРІРѕР±РѕРґРЅРѕ РёР· " << FormatBytes(g_sysInfo.totalDiskSpace.QuadPart) << L"</td></tr>";
        file << L"</table>";
        
        // РџСЂРѕРіСЂР°РјРјС‹
        file << L"<h2>РЈСЃС‚Р°РЅРѕРІР»РµРЅРЅС‹Рµ РїСЂРѕРіСЂР°РјРјС‹ (" << g_programs.size() << L")</h2>";
        file << L"<table>";
        file << L"<tr><th>РќР°Р·РІР°РЅРёРµ</th><th>Р’РµСЂСЃРёСЏ</th><th>РР·РґР°С‚РµР»СЊ</th><th>Р Р°Р·РјРµСЂ</th></tr>";
        
        for (const auto& prog : g_programs) {
            file << L"<tr>";
            file << L"<td>" << prog.name << L"</td>";
            file << L"<td>" << prog.version << L"</td>";
            file << L"<td>" << prog.publisher << L"</td>";
            file << L"<td>" << FormatBytes(prog.size) << L"</td>";
            file << L"</tr>";
        }
        file << L"</table>";
        
        file << L"</body></html>";
        file.close();
        
        AddLogMessage(L"РћС‚С‡РµС‚ СЃРѕС…СЂР°РЅРµРЅ: " + std::wstring(fileName), COLOR_SUCCESS);
        
        // РћС‚РєСЂС‹С‚СЊ РѕС‚С‡РµС‚
        ShellExecute(NULL, L"open", fileName, NULL, NULL, SW_SHOW);
    }
}

// Р—Р°РіСЂСѓР·РєР° СЃРїРёСЃРєР° РїСЂРѕРіСЂР°РјРј
void LoadPrograms() {
    g_programs = ProgramsModule::GetInstalledPrograms();
    AddLogMessage(L"РќР°Р№РґРµРЅРѕ РїСЂРѕРіСЂР°РјРј: " + std::to_wstring(g_programs.size()), COLOR_SUCCESS);
    
    // РџРѕРёСЃРє РєРѕРЅС„Р»РёРєС‚РѕРІ
    int conflicts = 0;
    for (auto& prog : g_programs) {
        if (ProgramsModule::IsConflictProgram(prog.name)) {
            prog.isConflict = true;
            conflicts++;
        }
    }
    
    if (conflicts > 0) {
        AddLogMessage(L"РќР°Р№РґРµРЅРѕ РєРѕРЅС„Р»РёРєС‚СѓСЋС‰РёС… РїСЂРѕРіСЂР°РјРј: " + std::to_wstring(conflicts), COLOR_WARNING);
    }
}

// Р—Р°РіСЂСѓР·РєР° СЃРїРёСЃРєР° СЃР»СѓР¶Р±
void LoadServices() {
    g_services = ServicesModule::GetServices();
    AddLogMessage(L"РќР°Р№РґРµРЅРѕ СЃР»СѓР¶Р±: " + std::to_wstring(g_services.size()), COLOR_SUCCESS);
}
// Просмотр отчета
void ViewReport() {
    // Поиск последнего HTML отчета
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile(L"*.html", &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        std::wstring latestReport = findData.cFileName;
        FindClose(hFind);
        
        // Открыть в браузере
        ShellExecute(NULL, L"open", latestReport.c_str(), NULL, NULL, SW_SHOW);
        AddLogMessage(L"Открыт отчет: " + latestReport, COLOR_SUCCESS);
    } else {
        MessageBox(hMainWnd, L"Отчеты не найдены. Сначала выполните диагностику.", 
                   L"Информация", MB_OK | MB_ICONINFORMATION);
    }
}

// Переключение содержимого вкладок
void ShowTabContent(int tabIndex) {
    // Временная заглушка
    std::wstring tabNames[] = {
        L"Главная", L"Диагностика", L"Очистка", 
        L"Программы", L"Службы", L"Сеть", L"Безопасность", L"Планировщик"
    };
    
    if (tabIndex >= 0 && tabIndex < 8) {
        SetWindowText(hStatusBar, (L"Вкладка: " + tabNames[tabIndex]).c_str());
    }
}
