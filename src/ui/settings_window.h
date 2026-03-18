#pragma once
#include <windows.h>
#include <vector>
#include <functional>
#include "../core/persistence.h"
#include "../core/data_collector.h"

namespace ui {

//-----------------------------------------------------------------------------
// SettingsWindow — displays current settings and allows editing
//-----------------------------------------------------------------------------
class SettingsWindow {
public:
    using SaveCallback = std::function<void(const core::AppSettings&)>;

    SettingsWindow() = default;
    ~SettingsWindow();

    bool create(HINSTANCE hInst, HWND parent = nullptr);
    void destroy();
    void show(const core::AppSettings& current);
    void hide();
    bool is_visible() const;

    void set_save_callback(SaveCallback cb) { save_cb_ = std::move(cb); }

private:
    static LRESULT CALLBACK wnd_proc_static(HWND, UINT, WPARAM, LPARAM);
    LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);

    void on_paint(HDC hdc);
    void on_save();
    void on_load_interfaces();

    void create_controls();
    void populate_controls(const core::AppSettings& s);
    void collect_settings(core::AppSettings& out);

    // Control IDs
    enum {
        ID_COMBO_IFACE       = 200,
        ID_COMBO_INTERVAL    = 201,
        ID_COMBO_POSITION    = 202,
        ID_COMBO_COLOR       = 203,
        ID_EDIT_DAILY_LIMIT  = 204,
        ID_EDIT_MONTHLY_LIMIT= 205,
        ID_EDIT_FONT_SIZE    = 206,
        ID_BTN_SAVE          = 207,
        ID_BTN_CANCEL        = 208,
        ID_CHECK_START_MIN   = 209,
        ID_CHECK_START_WIN   = 210,
    };

    static constexpr wchar_t kClassName[] = L"TrafficSettings_v3";
    static constexpr int kW = 400;
    static constexpr int kH = 480;

    HWND       hwnd_{ nullptr };
    HINSTANCE  hInst_{ nullptr };
    bool       registered_{ false };

    HFONT      font_{ nullptr };
    HFONT      font_bold_{ nullptr };

    // Cached interface list
    std::vector<core::NetworkInterface> interfaces_;

    SaveCallback save_cb_;
};

} // namespace ui
