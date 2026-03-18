#include "settings_window.h"
#include "theme.h"
#include "../utils/logger.h"
#include <commctrl.h>

namespace ui {

//=============================================================================
SettingsWindow::~SettingsWindow() {
    destroy();
    if (font_)      DeleteObject(font_);
    if (font_bold_) DeleteObject(font_bold_);
}

bool SettingsWindow::create(HINSTANCE hInst, HWND parent) {
    hInst_ = hInst;

    if (!registered_) {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = wnd_proc_static;
        wc.hInstance     = hInst;
        wc.lpszClassName = kClassName;
        wc.hbrBackground = CreateSolidBrush(theme().bg_primary);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassExW(&wc)) {
            LOG_ERROR("Failed to register settings window class");
            return false;
        }
        registered_ = true;
    }

    font_      = create_font(13);
    font_bold_ = create_font(13, true);

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kClassName, L"Traffic Monitor — Settings",
        style, CW_USEDEFAULT, CW_USEDEFAULT, kW, kH,
        parent, nullptr, hInst, this);

    if (!hwnd_) {
        LOG_ERROR("Failed to create settings window");
        return false;
    }

    create_controls();
    LOG_INFO("Settings window created");
    return true;
}

void SettingsWindow::destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void SettingsWindow::show(const core::AppSettings& current) {
    if (!hwnd_) return;
    on_load_interfaces();
    populate_controls(current);
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    SetForegroundWindow(hwnd_);
}

void SettingsWindow::hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

bool SettingsWindow::is_visible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

//=============================================================================
// Create child controls
//=============================================================================
void SettingsWindow::create_controls() {
    // Helper lambda to create labeled control rows
    auto label = [&](int x, int y, const wchar_t* text, int w = 160) -> HWND {
        return CreateWindowExW(0, L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x, y, w, 20, hwnd_, nullptr, hInst_, nullptr);
    };
    auto combo = [&](int x, int y, int id, int w = 210) -> HWND {
        return CreateWindowExW(0, L"COMBOBOX", nullptr,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
            x, y, w, 200, hwnd_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst_, nullptr);
    };
    auto edit = [&](int x, int y, int id, int w = 100) -> HWND {
        return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_NUMBER,
            x, y, w, 22, hwnd_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst_, nullptr);
    };
    auto check = [&](int x, int y, int id, const wchar_t* text) -> HWND {
        return CreateWindowExW(0, L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_FLAT,
            x, y, 240, 22, hwnd_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst_, nullptr);
    };

    int lx = 20, cx = 190, y = 20, dy = 38;

    // -- Interface
    label(lx, y, L"Network Interface:");
    combo(cx, y, ID_COMBO_IFACE, 180); y += dy;

    // -- Update interval
    label(lx, y, L"Update Interval:");
    HWND cb_int = combo(cx, y, ID_COMBO_INTERVAL, 180);
    SendMessageW(cb_int, CB_ADDSTRING, 0, (LPARAM)L"0.5 seconds");
    SendMessageW(cb_int, CB_ADDSTRING, 0, (LPARAM)L"1 second");
    SendMessageW(cb_int, CB_ADDSTRING, 0, (LPARAM)L"2 seconds");
    SendMessageW(cb_int, CB_ADDSTRING, 0, (LPARAM)L"5 seconds");
    y += dy;

    // -- Overlay position
    label(lx, y, L"Overlay Position:");
    HWND cb_pos = combo(cx, y, ID_COMBO_POSITION, 180);
    const wchar_t* positions[] = {
        L"Top Left", L"Top Center", L"Top Right",
        L"Middle Left", L"Middle Right",
        L"Bottom Left", L"Bottom Center", L"Bottom Right"
    };
    for (auto* p : positions)
        SendMessageW(cb_pos, CB_ADDSTRING, 0, (LPARAM)p);
    y += dy;

    // -- Text color
    label(lx, y, L"Overlay Text Color:");
    HWND cb_col = combo(cx, y, ID_COMBO_COLOR, 180);
    const wchar_t* colors[] = { L"Red", L"Green", L"Blue", L"White", L"Yellow" };
    for (auto* c : colors)
        SendMessageW(cb_col, CB_ADDSTRING, 0, (LPARAM)c);
    y += dy;

    // -- Font size
    label(lx, y, L"Overlay Font Size:");
    edit(cx, y, ID_EDIT_FONT_SIZE, 60); y += dy;

    // -- Daily limit
    label(lx, y, L"Daily Limit (MB, 0=off):");
    edit(cx, y, ID_EDIT_DAILY_LIMIT, 100); y += dy;

    // -- Monthly limit
    label(lx, y, L"Monthly Limit (MB, 0=off):");
    edit(cx, y, ID_EDIT_MONTHLY_LIMIT, 100); y += dy;

    // -- Checkboxes
    check(lx, y, ID_CHECK_START_MIN, L"Start minimized to tray");
    y += 28;
    check(lx, y, ID_CHECK_START_WIN, L"Start with Windows");
    y += 40;

    // -- Buttons
    CreateWindowExW(0, L"BUTTON", L"Save",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_FLAT,
        kW - 220, y, 90, 30,
        hwnd_, reinterpret_cast<HMENU>(ID_BTN_SAVE), hInst_, nullptr);

    CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_FLAT,
        kW - 120, y, 90, 30,
        hwnd_, reinterpret_cast<HMENU>(ID_BTN_CANCEL), hInst_, nullptr);

    // Set font on all children
    EnumChildWindows(hwnd_, [](HWND child, LPARAM lparam) -> BOOL {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(lparam), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(font_));
}

//=============================================================================
void SettingsWindow::on_load_interfaces() {
    HWND cb = GetDlgItem(hwnd_, ID_COMBO_IFACE);
    if (!cb) return;
    SendMessageW(cb, CB_RESETCONTENT, 0, 0);

    // First entries: aggregated types
    SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)L"All Interfaces");
    SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)L"Wi-Fi Only");
    SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)L"Ethernet Only");
    SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)L"Mobile Only");

    // Then specific adapters
    interfaces_ = core::DataCollector::enumerate_interfaces();
    for (const auto& iface : interfaces_) {
        std::wstring name = iface.name.empty() ? iface.description : iface.name;
        SendMessageW(cb, CB_ADDSTRING, 0, (LPARAM)name.c_str());
    }
}

void SettingsWindow::populate_controls(const core::AppSettings& s) {
    // Interface
    HWND cb_iface = GetDlgItem(hwnd_, ID_COMBO_IFACE);
    if (cb_iface) {
        int sel = static_cast<int>(s.iface_type);
        if (s.iface_type == core::InterfaceType::Specific) {
            // Find adapter in list
            sel = 4;
            for (int i = 0; i < static_cast<int>(interfaces_.size()); ++i) {
                if (interfaces_[i].index == s.specific_iface_index) {
                    sel = 4 + i;
                    break;
                }
            }
        }
        SendMessageW(cb_iface, CB_SETCURSEL, sel, 0);
    }

    // Interval
    HWND cb_int = GetDlgItem(hwnd_, ID_COMBO_INTERVAL);
    if (cb_int) {
        int sel = 1;
        switch (s.update_interval) {
            case core::UpdateInterval::Half:  sel = 0; break;
            case core::UpdateInterval::One:   sel = 1; break;
            case core::UpdateInterval::Two:   sel = 2; break;
            case core::UpdateInterval::Five:  sel = 3; break;
        }
        SendMessageW(cb_int, CB_SETCURSEL, sel, 0);
    }

    // Position
    HWND cb_pos = GetDlgItem(hwnd_, ID_COMBO_POSITION);
    if (cb_pos) SendMessageW(cb_pos, CB_SETCURSEL,
                              static_cast<int>(s.overlay_pos), 0);

    // Color
    HWND cb_col = GetDlgItem(hwnd_, ID_COMBO_COLOR);
    if (cb_col) SendMessageW(cb_col, CB_SETCURSEL,
                              static_cast<int>(s.text_color), 0);

    // Font size
    HWND ed_fs = GetDlgItem(hwnd_, ID_EDIT_FONT_SIZE);
    if (ed_fs) SetWindowTextW(ed_fs, std::to_wstring(s.font_size).c_str());

    // Limits
    HWND ed_dl = GetDlgItem(hwnd_, ID_EDIT_DAILY_LIMIT);
    if (ed_dl) SetWindowTextW(ed_dl, std::to_wstring(s.daily_limit_mb).c_str());

    HWND ed_ml = GetDlgItem(hwnd_, ID_EDIT_MONTHLY_LIMIT);
    if (ed_ml) SetWindowTextW(ed_ml, std::to_wstring(s.monthly_limit_mb).c_str());

    // Checkboxes
    HWND ch_min = GetDlgItem(hwnd_, ID_CHECK_START_MIN);
    if (ch_min) SendMessageW(ch_min, BM_SETCHECK,
                              s.start_minimized ? BST_CHECKED : BST_UNCHECKED, 0);
    HWND ch_win = GetDlgItem(hwnd_, ID_CHECK_START_WIN);
    if (ch_win) SendMessageW(ch_win, BM_SETCHECK,
                              s.start_with_windows ? BST_CHECKED : BST_UNCHECKED, 0);
}

void SettingsWindow::collect_settings(core::AppSettings& out) {
    // Interface
    HWND cb_iface = GetDlgItem(hwnd_, ID_COMBO_IFACE);
    if (cb_iface) {
        int sel = static_cast<int>(SendMessageW(cb_iface, CB_GETCURSEL, 0, 0));
        if (sel < 4) {
            out.iface_type = static_cast<core::InterfaceType>(sel);
        } else {
            out.iface_type = core::InterfaceType::Specific;
            int idx = sel - 4;
            if (idx >= 0 && idx < static_cast<int>(interfaces_.size())) {
                out.specific_iface_index = interfaces_[idx].index;
                out.specific_iface_name  = interfaces_[idx].name;
            }
        }
    }

    // Interval
    HWND cb_int = GetDlgItem(hwnd_, ID_COMBO_INTERVAL);
    if (cb_int) {
        int sel = static_cast<int>(SendMessageW(cb_int, CB_GETCURSEL, 0, 0));
        static const core::UpdateInterval intervals[] = {
            core::UpdateInterval::Half,
            core::UpdateInterval::One,
            core::UpdateInterval::Two,
            core::UpdateInterval::Five
        };
        if (sel >= 0 && sel < 4) out.update_interval = intervals[sel];
    }

    // Position
    HWND cb_pos = GetDlgItem(hwnd_, ID_COMBO_POSITION);
    if (cb_pos) {
        int sel = static_cast<int>(SendMessageW(cb_pos, CB_GETCURSEL, 0, 0));
        out.overlay_pos = static_cast<core::OverlayPosition>(sel);
    }

    // Color
    HWND cb_col = GetDlgItem(hwnd_, ID_COMBO_COLOR);
    if (cb_col) {
        int sel = static_cast<int>(SendMessageW(cb_col, CB_GETCURSEL, 0, 0));
        out.text_color = static_cast<core::TextColor>(sel);
    }

    // Font size
    wchar_t buf[16] = {};
    HWND ed_fs = GetDlgItem(hwnd_, ID_EDIT_FONT_SIZE);
    if (ed_fs) {
        GetWindowTextW(ed_fs, buf, 16);
        try { out.font_size = std::stoi(buf); } catch (...) {}
        out.font_size = std::max(8, std::min(32, out.font_size));
    }

    HWND ed_dl = GetDlgItem(hwnd_, ID_EDIT_DAILY_LIMIT);
    if (ed_dl) {
        GetWindowTextW(ed_dl, buf, 16);
        try { out.daily_limit_mb = std::stoull(buf); } catch (...) {}
    }

    HWND ed_ml = GetDlgItem(hwnd_, ID_EDIT_MONTHLY_LIMIT);
    if (ed_ml) {
        GetWindowTextW(ed_ml, buf, 16);
        try { out.monthly_limit_mb = std::stoull(buf); } catch (...) {}
    }

    HWND ch_min = GetDlgItem(hwnd_, ID_CHECK_START_MIN);
    if (ch_min) out.start_minimized =
        (SendMessageW(ch_min, BM_GETCHECK, 0, 0) == BST_CHECKED);

    HWND ch_win = GetDlgItem(hwnd_, ID_CHECK_START_WIN);
    if (ch_win) out.start_with_windows =
        (SendMessageW(ch_win, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

void SettingsWindow::on_save() {
    core::AppSettings settings;
    collect_settings(settings);
    if (save_cb_) save_cb_(settings);
    hide();
}

//=============================================================================
LRESULT CALLBACK SettingsWindow::wnd_proc_static(HWND hwnd, UINT msg,
                                                   WPARAM wp, LPARAM lp) {
    SettingsWindow* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<SettingsWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<SettingsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) return self->wnd_proc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT SettingsWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CLOSE:
            hide();
            return 0;

        case WM_DESTROY:
            hwnd_ = nullptr;
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            on_paint(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_BTN_SAVE:   on_save(); break;
                case ID_BTN_CANCEL: hide();    break;
            }
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkColor(hdc, theme().bg_secondary);
            SetTextColor(hdc, theme().text_primary);
            return reinterpret_cast<LRESULT>(theme().br_secondary);
        }

        case WM_CTLCOLORBTN: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkColor(hdc, theme().bg_tertiary);
            SetTextColor(hdc, theme().text_primary);
            return reinterpret_cast<LRESULT>(theme().br_tertiary);
        }

        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void SettingsWindow::on_paint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH bg = CreateSolidBrush(theme().bg_primary);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    // Section labels
    SelectObject(hdc, font_bold_);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme().text_primary);

    RECT title_rc = { 10, 2, rc.right - 10, 20 };
    DrawTextW(hdc, L"Settings", -1, &title_rc, DT_LEFT | DT_TOP | DT_SINGLELINE);
}

} // namespace ui
