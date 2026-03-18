#include "overlay_window.h"
#include "theme.h"
#include "../utils/win_helpers.h"
#include "../utils/format_helpers.h"
#include "../utils/logger.h"

namespace ui {

OverlayWindow::~OverlayWindow() {
    destroy();
}

//=============================================================================
// Create
//=============================================================================
bool OverlayWindow::create(HINSTANCE hInst) {
    hInst_ = hInst;

    if (!registered_) {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = wnd_proc_static;
        wc.hInstance     = hInst;
        wc.lpszClassName = kClassName;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassExW(&wc)) {
            LOG_ERROR("Failed to register overlay class");
            return false;
        }
        registered_ = true;
    }

    // Layered window: WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW
    DWORD ex_style = WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    DWORD style    = WS_POPUP;

    hwnd_ = CreateWindowExW(
        ex_style, kClassName, L"Traffic Overlay",
        style, 0, 0, 200, 40,
        nullptr, nullptr, hInst, this);

    if (!hwnd_) {
        LOG_ERROR("Failed to create overlay window");
        return false;
    }

    // Initial render
    render();

    if (visible_) show();
    LOG_INFO("Overlay window created");
    return true;
}

void OverlayWindow::destroy() {
    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

//=============================================================================
// Update data and re-render
//=============================================================================
void OverlayWindow::update(const core::TrafficSnapshot& snap,
                            const std::wstring& iface_name) {
    rx_speed_  = snap.rx_speed;
    tx_speed_  = snap.tx_speed;
    iface_name_ = iface_name;
    if (hwnd_ && visible_) render();
}

//=============================================================================
// Show / Hide
//=============================================================================
void OverlayWindow::show() {
    visible_ = true;
    if (hwnd_) {
        ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
        render();
    }
}

void OverlayWindow::hide() {
    visible_ = false;
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

//=============================================================================
// Apply settings
//=============================================================================
void OverlayWindow::apply_settings(const core::AppSettings& s) {
    position_   = s.overlay_pos;
    font_size_  = s.font_size;
    offset_x_   = s.overlay_offset_x;
    offset_y_   = s.overlay_offset_y;

    switch (s.text_color) {
        case core::TextColor::Red:    text_color_ = theme().overlay_red;    break;
        case core::TextColor::Green:  text_color_ = theme().overlay_green;  break;
        case core::TextColor::Blue:   text_color_ = theme().overlay_blue;   break;
        case core::TextColor::White:  text_color_ = theme().overlay_white;  break;
        case core::TextColor::Yellow: text_color_ = theme().overlay_yellow; break;
    }

    if (hwnd_) {
        update_position();
        render();
    }
}

//=============================================================================
// Render — draws into DIB and calls UpdateLayeredWindow
//=============================================================================
void OverlayWindow::render() {
    if (!hwnd_) return;

    // Build display string: ↓ rx  ↑ tx | iface
    std::wstring rx_str = utils::format_speed(rx_speed_);
    std::wstring tx_str = utils::format_speed(tx_speed_);
    // Full line: "↓ 1.2M/s  ↑ 256K/s | Wi-Fi"
    std::wstring line1 = std::wstring(L"\u2193 ") + rx_str + L"  \u2191 " + tx_str;
    std::wstring line2 = iface_name_;

    // Create screen DC
    HDC screen_dc = GetDC(nullptr);

    // Measure text size
    HFONT font = ui::create_mono_font(font_size_, false);
    HDC mem_dc  = CreateCompatibleDC(screen_dc);
    HFONT old_f = static_cast<HFONT>(SelectObject(mem_dc, font));

    SIZE sz1{}, sz2{};
    GetTextExtentPoint32W(mem_dc, line1.c_str(), static_cast<int>(line1.size()), &sz1);
    GetTextExtentPoint32W(mem_dc, line2.c_str(), static_cast<int>(line2.size()), &sz2);

    int margin = 6;
    int w = std::max(sz1.cx, sz2.cx) + margin * 2 + 4; // +4 for shadow
    int h = sz1.cy + sz2.cy + margin * 2 + 4;

    // Recreate DIB if size changed
    if (w != dib_w_ || h != dib_h_) {
        dib_.create(screen_dc, w, h);
        dib_w_ = w;
        dib_h_ = h;
        // Resize window
        POINT pos = calc_position();
        SetWindowPos(hwnd_, HWND_TOPMOST,
                     pos.x, pos.y, w, h,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    // Clear DIB to transparent
    dib_.clear();

    // Render into memory DC backed by DIB
    HBITMAP old_bm = static_cast<HBITMAP>(SelectObject(mem_dc, dib_.bitmap()));

    // Draw text with shadow
    SetBkMode(mem_dc, TRANSPARENT);

    // Line 1: ↓ rx ↑ tx
    // Coloured arrows: blue for down, green for up
    // For simplicity, draw entire line in user-selected color with shadow
    draw_text_shadow(mem_dc, line1.c_str(), margin, margin,
                     text_color_, RGB(0, 0, 0), 2);

    // Line 2: interface name (dimmer)
    COLORREF dim = RGB(
        static_cast<BYTE>(GetRValue(text_color_) * 60 / 100),
        static_cast<BYTE>(GetGValue(text_color_) * 60 / 100),
        static_cast<BYTE>(GetBValue(text_color_) * 60 / 100));
    draw_text_shadow(mem_dc, line2.c_str(), margin, margin + sz1.cy + 2,
                     dim, RGB(0, 0, 0), 1);

    // Color-code the arrows explicitly
    // Re-draw just the arrows in their specific colors
    COLORREF rx_col = RGB(80, 160, 255); // down = blue
    COLORREF tx_col = RGB(80, 220, 120); // up   = green

    // ↓ arrow
    draw_text_shadow(mem_dc, L"\u2193", margin, margin, rx_col, RGB(0,0,0), 2);

    // Measure ↓ and space
    SIZE arrow_sz{};
    GetTextExtentPoint32W(mem_dc, L"\u2193 ", 2, &arrow_sz);

    // Speed values after arrow
    std::wstring rx_part = rx_str;
    draw_text_shadow(mem_dc, rx_part.c_str(), margin + arrow_sz.cx, margin,
                     text_color_, RGB(0,0,0), 2);

    // ↑ arrow (after rx)
    SIZE rx_part_sz{};
    GetTextExtentPoint32W(mem_dc, (std::wstring(L"\u2193 ") + rx_part + L"  ").c_str(),
                          -1, &rx_part_sz);
    draw_text_shadow(mem_dc, L"\u2191",
                     margin + rx_part_sz.cx, margin,
                     tx_col, RGB(0,0,0), 2);

    SIZE up_arrow_sz{};
    GetTextExtentPoint32W(mem_dc, L"\u2191 ", 2, &up_arrow_sz);
    draw_text_shadow(mem_dc, tx_str.c_str(),
                     margin + rx_part_sz.cx + up_arrow_sz.cx, margin,
                     text_color_, RGB(0,0,0), 2);

    // Restore DC
    SelectObject(mem_dc, old_f);
    SelectObject(mem_dc, old_bm);
    DeleteObject(font);

    // UpdateLayeredWindow
    POINT pt_src = { 0, 0 };
    POINT pt_dst = calc_position();
    SIZE  sz_wnd = { w, h };
    BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    SetWindowPos(hwnd_, HWND_TOPMOST,
                 pt_dst.x, pt_dst.y, w, h,
                 SWP_NOACTIVATE | SWP_NOZORDER);

    UpdateLayeredWindow(hwnd_, screen_dc, &pt_dst, &sz_wnd,
                        mem_dc, &pt_src, 0, &blend, ULW_ALPHA);

    DeleteDC(mem_dc);
    ReleaseDC(nullptr, screen_dc);
}

//=============================================================================
// Draw text with drop shadow using pre-multiplied alpha
//=============================================================================
void OverlayWindow::draw_text_shadow(HDC hdc, const wchar_t* text,
                                      int x, int y,
                                      COLORREF color, COLORREF shadow_color,
                                      int shadow_offset) {
    // Shadow
    SetTextColor(hdc, shadow_color);
    TextOutW(hdc, x + shadow_offset, y + shadow_offset, text,
             static_cast<int>(wcslen(text)));

    // Main text
    SetTextColor(hdc, color);
    TextOutW(hdc, x, y, text, static_cast<int>(wcslen(text)));
}

//=============================================================================
// Calculate overlay position on screen
//=============================================================================
POINT OverlayWindow::calc_position() const {
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    // Use work area to avoid taskbar overlap
    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    sw = work.right  - work.left;
    sh = work.bottom - work.top;

    int x = work.left;
    int y = work.top;

    switch (position_) {
        case core::OverlayPosition::TopLeft:
            x = work.left  + offset_x_;
            y = work.top   + offset_y_;
            break;
        case core::OverlayPosition::TopCenter:
            x = work.left  + (sw - dib_w_) / 2;
            y = work.top   + offset_y_;
            break;
        case core::OverlayPosition::TopRight:
            x = work.right  - dib_w_ - offset_x_;
            y = work.top    + offset_y_;
            break;
        case core::OverlayPosition::MiddleLeft:
            x = work.left   + offset_x_;
            y = work.top    + (sh - dib_h_) / 2;
            break;
        case core::OverlayPosition::MiddleRight:
            x = work.right  - dib_w_ - offset_x_;
            y = work.top    + (sh - dib_h_) / 2;
            break;
        case core::OverlayPosition::BottomLeft:
            x = work.left   + offset_x_;
            y = work.bottom - dib_h_ - offset_y_;
            break;
        case core::OverlayPosition::BottomCenter:
            x = work.left   + (sw - dib_w_) / 2;
            y = work.bottom - dib_h_ - offset_y_;
            break;
        case core::OverlayPosition::BottomRight:
            x = work.right  - dib_w_ - offset_x_;
            y = work.bottom - dib_h_ - offset_y_;
            break;
    }
    return { x, y };
}

void OverlayWindow::update_position() {
    if (!hwnd_) return;
    POINT pos = calc_position();
    SetWindowPos(hwnd_, HWND_TOPMOST,
                 pos.x, pos.y, dib_w_, dib_h_,
                 SWP_NOACTIVATE | SWP_NOSIZE);
}

//=============================================================================
// Window procedure
//=============================================================================
LRESULT CALLBACK OverlayWindow::wnd_proc_static(HWND hwnd, UINT msg,
                                                   WPARAM wp, LPARAM lp) {
    OverlayWindow* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<OverlayWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<OverlayWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) return self->wnd_proc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT OverlayWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_DESTROY:
            hwnd_ = nullptr;
            return 0;
        case WM_NCHITTEST:
            // Allow dragging with left mouse
            return HTCAPTION;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace ui
