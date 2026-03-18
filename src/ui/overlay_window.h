#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include "../core/persistence.h"
#include "../core/data_collector.h"

namespace ui {

//-----------------------------------------------------------------------------
// OverlayWindow — transparent layered window showing traffic speed
// Renders colored text with drop shadow, no background
//-----------------------------------------------------------------------------
class OverlayWindow {
public:
    OverlayWindow() = default;
    ~OverlayWindow();

    bool create(HINSTANCE hInst);
    void destroy();

    // Update displayed data
    void update(const core::TrafficSnapshot& snap,
                const std::wstring& iface_name);

    // Show/hide
    void show();
    void hide();
    bool is_visible() const { return visible_; }
    void toggle_visibility() { visible_ ? hide() : show(); }

    // Apply settings
    void apply_settings(const core::AppSettings& settings);

    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK wnd_proc_static(HWND, UINT, WPARAM, LPARAM);
    LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);

    void render();
    void update_position();

    POINT calc_position() const;

    // Draw text with shadow into the DIB
    void draw_text_shadow(HDC hdc, const wchar_t* text, int x, int y,
                           COLORREF color, COLORREF shadow_color,
                           int shadow_offset = 2);

    static constexpr wchar_t kClassName[] = L"TrafficOverlay_v3";

    HWND    hwnd_{ nullptr };
    HINSTANCE hInst_{ nullptr };

    // Current data
    double  rx_speed_{ 0.0 };
    double  tx_speed_{ 0.0 };
    std::wstring iface_name_;

    // Settings
    core::OverlayPosition position_{ core::OverlayPosition::TopRight };
    COLORREF              text_color_{ RGB(230, 230, 240) };
    int                   font_size_{ 14 };
    int                   offset_x_{ 10 };
    int                   offset_y_{ 10 };

    bool    visible_{ true };
    bool    registered_{ false };

    // DIB for layered window rendering
    utils::dib_section dib_;
    int                dib_w_{ 0 };
    int                dib_h_{ 0 };
};

} // namespace ui
