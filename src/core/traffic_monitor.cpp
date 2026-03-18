#include "traffic_monitor.h"
#include "../utils/logger.h"
#include "../utils/format_helpers.h"
#include "../ui/theme.h"
#include <shellapi.h>

// Link shell for APPDATA path
#include <shlobj.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif

namespace core {

TrafficMonitor* TrafficMonitor::instance_ = nullptr;

//=============================================================================
TrafficMonitor::~TrafficMonitor() {
    shutdown();
}

bool TrafficMonitor::initialize(HINSTANCE hInst, bool start_minimized) {
    instance_ = this;
    hInst_    = hInst;

    LOG_INFO("Initializing TrafficMonitor v3.0");

    // Init theme brushes
    ui::theme().create_brushes();

    // Persistence
    persistence_ = std::make_unique<Persistence>();
    if (!persistence_->initialize()) {
        LOG_ERROR("Persistence initialization failed");
        return false;
    }

    // Load settings
    if (!persistence_->load_settings(settings_)) {
        LOG_WARN("Failed to load settings, using defaults");
    }

    // Logger to file
    Logger::instance().set_file(persistence_->data_dir() + L"\\traffic.log");

    // Statistics
    statistics_ = std::make_unique<Statistics>();
    statistics_->set_daily_limit(settings_.daily_limit_mb * 1024 * 1024);
    statistics_->set_monthly_limit(settings_.monthly_limit_mb * 1024 * 1024);
    statistics_->set_alert_callback([this](LimitAlert d, LimitAlert m) {
        on_limit_alert(d, m);
    });

    // Load saved stats
    std::map<uint32_t, DayRecord> saved_days;
    if (persistence_->load_stats(saved_days)) {
        statistics_->load_days(std::move(saved_days));
    }

    // Data collector
    collector_ = std::make_unique<DataCollector>();
    configure_data_collector();
    collector_->set_callback([this](const TrafficSnapshot& snap) {
        on_traffic_data(snap);
    });
    if (!collector_->start()) {
        LOG_ERROR("DataCollector start failed");
        return false;
    }

    // UI windows
    overlay_ = std::make_unique<ui::OverlayWindow>();
    if (!overlay_->create(hInst)) {
        LOG_ERROR("Overlay window creation failed");
        return false;
    }
    overlay_->apply_settings(settings_);

    stats_wnd_ = std::make_unique<ui::StatsWindow>(*statistics_, *persistence_);
    if (!stats_wnd_->create(hInst)) {
        LOG_ERROR("Stats window creation failed");
        return false;
    }

    settings_wnd_ = std::make_unique<ui::SettingsWindow>();
    settings_wnd_->set_save_callback([this](const AppSettings& s) {
        apply_settings(s);
    });
    if (!settings_wnd_->create(hInst)) {
        LOG_ERROR("Settings window creation failed");
        return false;
    }

    // Message window
    if (!create_message_window()) {
        LOG_ERROR("Message window creation failed");
        return false;
    }

    // Tray icon
    if (!create_tray_icon()) {
        LOG_WARN("Failed to create tray icon");
    }

    // Hotkeys
    if (!register_hotkeys()) {
        LOG_WARN("Failed to register some hotkeys");
    }

    // Auto-save stats every 5 minutes
    persistence_->start_autosave(*statistics_, 5);

    // Update timer for overlay (redundant with collector callback, but for UI refresh)
    SetTimer(msg_wnd_, TIMER_UPDATE_OVERLAY,
             static_cast<UINT>(settings_.update_interval), nullptr);

    running_ = true;

    if (start_minimized || settings_.start_minimized) {
        overlay_->hide();
    }

    LOG_INFO("TrafficMonitor initialized successfully");
    return true;
}

//=============================================================================
int TrafficMonitor::run() {
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

//=============================================================================
void TrafficMonitor::shutdown() {
    if (!running_) return;
    running_ = false;

    LOG_INFO("Shutting down...");

    // Stop collector
    if (collector_) collector_->stop();

    // Save stats
    if (statistics_ && persistence_) {
        persistence_->save_stats(statistics_->raw_days());
        persistence_->stop_autosave();
    }

    // Save settings
    if (persistence_) {
        persistence_->save_settings(settings_);
    }

    // Unregister hotkeys
    unregister_hotkeys();

    // Remove tray icon
    destroy_tray_icon();

    // Destroy windows
    if (overlay_)      overlay_->destroy();
    if (stats_wnd_)    stats_wnd_->destroy();
    if (settings_wnd_) settings_wnd_->destroy();

    if (msg_wnd_) {
        KillTimer(msg_wnd_, TIMER_UPDATE_OVERLAY);
        DestroyWindow(msg_wnd_);
        msg_wnd_ = nullptr;
    }

    ui::theme().destroy_brushes();

    LOG_INFO("Shutdown complete");
    instance_ = nullptr;
}

//=============================================================================
// Message window
//=============================================================================
bool TrafficMonitor::create_message_window() {
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = msg_wnd_proc;
    wc.hInstance     = hInst_;
    wc.lpszClassName = kMsgWndClass;
    RegisterClassExW(&wc);

    msg_wnd_ = CreateWindowExW(0, kMsgWndClass, L"",
        WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInst_, nullptr);

    return msg_wnd_ != nullptr;
}

LRESULT CALLBACK TrafficMonitor::msg_wnd_proc(HWND hwnd, UINT msg,
                                               WPARAM wp, LPARAM lp) {
    if (!instance_) return DefWindowProcW(hwnd, msg, wp, lp);

    switch (msg) {
        case WM_HOTKEY:
            switch (wp) {
                case HOTKEY_TOGGLE_OVERLAY: instance_->on_toggle_overlay(); break;
                case HOTKEY_SHOW_STATS:     instance_->on_show_stats();     break;
                case HOTKEY_SHOW_SETTINGS:  instance_->on_show_settings();  break;
                case HOTKEY_QUIT:           instance_->on_quit();           break;
            }
            return 0;

        case WM_TRAY_ICON:
            switch (lp) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                    instance_->show_tray_menu();
                    break;
                case WM_LBUTTONDBLCLK:
                    instance_->on_show_stats();
                    break;
            }
            return 0;

        case WM_TIMER:
            if (wp == TIMER_UPDATE_OVERLAY) {
                // Refresh stats window if open
                if (instance_->stats_wnd_ && instance_->stats_wnd_->is_visible())
                    instance_->stats_wnd_->refresh();
            }
            return 0;

        case WM_USER + 2: {
            // Posted by DataCollector callback — update overlay from main thread
            if (instance_->overlay_ && instance_->overlay_->is_visible()) {
                TrafficSnapshot snap = instance_->collector_->current_snapshot();
                instance_->overlay_->update(snap, instance_->iface_display_name_);
            }
            instance_->update_tray_tooltip();
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

//=============================================================================
// Tray icon
//=============================================================================
bool TrafficMonitor::create_tray_icon() {
    tray_icon_ = LoadIconW(hInst_, MAKEINTRESOURCEW(1));
    if (!tray_icon_) {
        // Use a standard system icon as fallback
        tray_icon_ = LoadIconW(nullptr, IDI_APPLICATION);
    }

    NOTIFYICONDATAW nid{};
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = msg_wnd_;
    nid.uID              = kTrayIconId;
    nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY_ICON;
    nid.hIcon            = tray_icon_;
    wcscpy_s(nid.szTip, L"Traffic Monitor");

    if (!Shell_NotifyIconW(NIM_ADD, &nid)) {
        LOG_ERROR("Shell_NotifyIconW NIM_ADD failed");
        return false;
    }

    // Use NOTIFYICON_VERSION_4 for improved balloon and tooltip
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);

    tray_created_ = true;
    LOG_INFO("Tray icon created");
    return true;
}

void TrafficMonitor::destroy_tray_icon() {
    if (!tray_created_) return;
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = msg_wnd_;
    nid.uID    = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
    tray_created_ = false;
}

void TrafficMonitor::update_tray_tooltip() {
    if (!tray_created_) return;
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(nid);
    nid.hWnd   = msg_wnd_;
    nid.uID    = kTrayIconId;
    nid.uFlags = NIF_TIP;

    std::wstring tip = std::wstring(L"Traffic Monitor\n\u2193") +
        utils::format_speed(last_rx_) + L" \u2191" +
        utils::format_speed(last_tx_);
    wcscpy_s(nid.szTip, tip.c_str());
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrafficMonitor::show_tray_menu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1,
        overlay_->is_visible() ? L"Hide Overlay" : L"Show Overlay");
    AppendMenuW(menu, MF_STRING, 2, L"Statistics...");
    AppendMenuW(menu, MF_STRING, 3, L"Settings...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 4, L"Exit");

    // Needed for correct menu close behavior
    SetForegroundWindow(msg_wnd_);

    POINT pt;
    GetCursorPos(&pt);
    int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                             pt.x, pt.y, 0, msg_wnd_, nullptr);
    DestroyMenu(menu);

    switch (cmd) {
        case 1: on_toggle_overlay(); break;
        case 2: on_show_stats();     break;
        case 3: on_show_settings();  break;
        case 4: on_quit();           break;
    }
}

//=============================================================================
// Hotkey handlers
//=============================================================================
bool TrafficMonitor::register_hotkeys() {
    bool ok = true;
    // Ctrl+Shift+H
    ok &= RegisterHotKey(msg_wnd_, HOTKEY_TOGGLE_OVERLAY,
                         MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'H');
    // Ctrl+Shift+S
    ok &= RegisterHotKey(msg_wnd_, HOTKEY_SHOW_STATS,
                         MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'S');
    // Ctrl+Shift+P
    ok &= RegisterHotKey(msg_wnd_, HOTKEY_SHOW_SETTINGS,
                         MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'P');
    // Ctrl+Shift+Q
    ok &= RegisterHotKey(msg_wnd_, HOTKEY_QUIT,
                         MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, 'Q');
    return ok;
}

void TrafficMonitor::unregister_hotkeys() {
    if (!msg_wnd_) return;
    UnregisterHotKey(msg_wnd_, HOTKEY_TOGGLE_OVERLAY);
    UnregisterHotKey(msg_wnd_, HOTKEY_SHOW_STATS);
    UnregisterHotKey(msg_wnd_, HOTKEY_SHOW_SETTINGS);
    UnregisterHotKey(msg_wnd_, HOTKEY_QUIT);
}

void TrafficMonitor::on_toggle_overlay() {
    if (overlay_) overlay_->toggle_visibility();
    settings_.overlay_visible = overlay_->is_visible();
}

void TrafficMonitor::on_show_stats() {
    if (stats_wnd_) stats_wnd_->show();
}

void TrafficMonitor::on_show_settings() {
    if (settings_wnd_) settings_wnd_->show(settings_);
}

void TrafficMonitor::on_quit() {
    shutdown();
    if (msg_wnd_) PostMessageW(msg_wnd_, WM_DESTROY, 0, 0);
}

//=============================================================================
// Data callback
//=============================================================================
void TrafficMonitor::on_traffic_data(const TrafficSnapshot& snap) {
    // Update statistics
    if (statistics_) {
        statistics_->update(snap.rx_delta, snap.tx_delta,
                            snap.rx_speed, snap.tx_speed);
    }

    last_rx_ = snap.rx_speed;
    last_tx_ = snap.tx_speed;

    // Update overlay from main thread via PostMessage
    if (overlay_ && msg_wnd_) {
        // Encode speeds as WM_USER message payload
        // We use a simple approach: post WM_TIMER-alike via PostMessage
        PostMessageW(msg_wnd_, WM_USER + 2, 0, 0);
    }
}

//=============================================================================
// Apply settings
//=============================================================================
void TrafficMonitor::apply_settings(const AppSettings& s) {
    settings_ = s;

    // Reconfigure limits
    if (statistics_) {
        statistics_->set_daily_limit(s.daily_limit_mb * 1024 * 1024);
        statistics_->set_monthly_limit(s.monthly_limit_mb * 1024 * 1024);
    }

    // Reconfigure collector
    if (collector_) {
        collector_->stop();
        configure_data_collector();
        collector_->start();
    }

    // Apply overlay settings
    if (overlay_) {
        overlay_->apply_settings(s);
        if (!s.overlay_visible) overlay_->hide();
        else overlay_->show();
    }

    // Reset update timer
    if (msg_wnd_) {
        KillTimer(msg_wnd_, TIMER_UPDATE_OVERLAY);
        SetTimer(msg_wnd_, TIMER_UPDATE_OVERLAY,
                 static_cast<UINT>(s.update_interval), nullptr);
    }

    // Handle start-with-windows
    if (s.start_with_windows) {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(hInst_, exe_path, MAX_PATH);
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"TrafficMonitor", 0, REG_SZ,
                reinterpret_cast<const BYTE*>(exe_path),
                static_cast<DWORD>((wcslen(exe_path) + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
        }
    } else {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegDeleteValueW(hKey, L"TrafficMonitor");
            RegCloseKey(hKey);
        }
    }

    // Save settings
    if (persistence_) {
        persistence_->save_settings(s);
    }
}

void TrafficMonitor::configure_data_collector() {
    switch (settings_.iface_type) {
        case InterfaceType::All:
            collector_->set_filter(InterfaceFilter::All);
            iface_display_name_ = L"All";
            break;
        case InterfaceType::WiFi:
            collector_->set_filter(InterfaceFilter::WiFi);
            iface_display_name_ = L"Wi-Fi";
            break;
        case InterfaceType::Ethernet:
            collector_->set_filter(InterfaceFilter::Ethernet);
            iface_display_name_ = L"Ethernet";
            break;
        case InterfaceType::Mobile:
            collector_->set_filter(InterfaceFilter::Mobile);
            iface_display_name_ = L"Mobile";
            break;
        case InterfaceType::Specific:
            collector_->set_filter(InterfaceFilter::Specific);
            collector_->set_interface_index(settings_.specific_iface_index);
            iface_display_name_ = settings_.specific_iface_name;
            break;
    }
    collector_->set_interval_ms(static_cast<int>(settings_.update_interval));
}

void TrafficMonitor::on_limit_alert(LimitAlert daily, LimitAlert monthly) {
    // Show balloon notification
    if (!tray_created_) return;

    NOTIFYICONDATAW nid{};
    nid.cbSize      = sizeof(nid);
    nid.hWnd        = msg_wnd_;
    nid.uID         = kTrayIconId;
    nid.uFlags      = NIF_INFO;
    nid.dwInfoFlags = NIIF_WARNING;
    nid.uTimeout    = 5000;
    wcscpy_s(nid.szInfoTitle, L"Traffic Monitor — Limit Alert");

    if (daily == LimitAlert::Exceeded) {
        wcscpy_s(nid.szInfo, L"Daily traffic limit exceeded!");
    } else if (daily == LimitAlert::Approaching) {
        wcscpy_s(nid.szInfo, L"Daily traffic limit at 80%");
    } else if (monthly == LimitAlert::Exceeded) {
        wcscpy_s(nid.szInfo, L"Monthly traffic limit exceeded!");
    } else if (monthly == LimitAlert::Approaching) {
        wcscpy_s(nid.szInfo, L"Monthly traffic limit at 80%");
    } else {
        return;
    }

    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

} // namespace core
