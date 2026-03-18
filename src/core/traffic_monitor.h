#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "data_collector.h"
#include "statistics.h"
#include "persistence.h"
#include "../ui/overlay_window.h"
#include "../ui/stats_window.h"
#include "../ui/settings_window.h"

namespace core {

// Tray icon message
constexpr UINT WM_TRAY_ICON = WM_USER + 1;

// Hotkey IDs
constexpr int HOTKEY_TOGGLE_OVERLAY = 1;
constexpr int HOTKEY_SHOW_STATS     = 2;
constexpr int HOTKEY_SHOW_SETTINGS  = 3;
constexpr int HOTKEY_QUIT           = 4;

// Timer IDs
constexpr UINT TIMER_UPDATE_OVERLAY = 1;
constexpr UINT TIMER_SAVE_STATS     = 2;

//-----------------------------------------------------------------------------
// TrafficMonitor — main application class
// Owns all subsystems and coordinates their interaction
//-----------------------------------------------------------------------------
class TrafficMonitor {
public:
    TrafficMonitor() = default;
    ~TrafficMonitor();

    // Initialize all subsystems
    bool initialize(HINSTANCE hInst, bool start_minimized = false);

    // Main message loop
    int run();

    // Graceful shutdown
    void shutdown();

    // Access from static callback
    static TrafficMonitor* instance() { return instance_; }

private:
    // Window procedure for hidden message window
    static LRESULT CALLBACK msg_wnd_proc(HWND, UINT, WPARAM, LPARAM);

    // Setup helpers
    bool create_message_window();
    bool register_hotkeys();
    void unregister_hotkeys();
    bool create_tray_icon();
    void destroy_tray_icon();
    void update_tray_tooltip();
    void show_tray_menu();

    // Hotkey handlers
    void on_toggle_overlay();
    void on_show_stats();
    void on_show_settings();
    void on_quit();

    // Data callback (called from background thread)
    void on_traffic_data(const TrafficSnapshot& snap);

    // Settings
    void apply_settings(const AppSettings& settings);
    void configure_data_collector();

    // Limit notification
    void on_limit_alert(LimitAlert daily, LimitAlert monthly);

    static constexpr wchar_t kMsgWndClass[] = L"TrafficMonitor_MsgWnd";
    static constexpr UINT    kTrayIconId    = 1;

    HINSTANCE  hInst_{ nullptr };
    HWND       msg_wnd_{ nullptr };
    HICON      tray_icon_{ nullptr };
    bool       tray_created_{ false };
    bool       running_{ false };

    // Settings
    AppSettings settings_;

    // Core subsystems
    std::unique_ptr<DataCollector>  collector_;
    std::unique_ptr<Statistics>     statistics_;
    std::unique_ptr<Persistence>    persistence_;

    // UI
    std::unique_ptr<ui::OverlayWindow>   overlay_;
    std::unique_ptr<ui::StatsWindow>     stats_wnd_;
    std::unique_ptr<ui::SettingsWindow>  settings_wnd_;

    // Current speed (for tray tooltip)
    double last_rx_{ 0.0 };
    double last_tx_{ 0.0 };
    std::wstring iface_display_name_;

    static TrafficMonitor* instance_;
};

} // namespace core
