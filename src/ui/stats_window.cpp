#include "stats_window.h"
#include "theme.h"
#include "../utils/format_helpers.h"
#include "../utils/logger.h"
#include <cstdio>
#include <commctrl.h>
#include <commdlg.h>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#endif

namespace ui {

static const wchar_t* kTabNames[] = { L"Today", L"Month", L"All Time", L"Graph" };

//=============================================================================
StatsWindow::~StatsWindow() {
    destroy();
    if (font_normal_) DeleteObject(font_normal_);
    if (font_bold_)   DeleteObject(font_bold_);
    if (font_small_)  DeleteObject(font_small_);
    if (font_large_)  DeleteObject(font_large_);
}

bool StatsWindow::create(HINSTANCE hInst, HWND parent) {
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
            LOG_ERROR("Failed to register stats window class");
            return false;
        }
        registered_ = true;
    }

    font_normal_ = create_font(13);
    font_bold_   = create_font(13, true);
    font_small_  = create_font(11);
    font_large_  = create_font(22, true);

    DWORD style    = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
    DWORD ex_style = WS_EX_APPWINDOW;

    hwnd_ = CreateWindowExW(ex_style, kClassName, L"Traffic Monitor — Statistics",
        style, CW_USEDEFAULT, CW_USEDEFAULT, kMinW, kMinH,
        parent, nullptr, hInst, this);

    if (!hwnd_) {
        LOG_ERROR("Failed to create stats window");
        return false;
    }

    // Export buttons
    CreateWindowExW(0, L"BUTTON", L"Export CSV",
        WS_CHILD | WS_VISIBLE | BS_FLAT,
        kMinW - 220, kMinH - 44, 90, 28,
        hwnd_, reinterpret_cast<HMENU>(ID_EXPORT_CSV), hInst, nullptr);

    CreateWindowExW(0, L"BUTTON", L"Export TXT",
        WS_CHILD | WS_VISIBLE | BS_FLAT,
        kMinW - 120, kMinH - 44, 90, 28,
        hwnd_, reinterpret_cast<HMENU>(ID_EXPORT_TXT), hInst, nullptr);

    LOG_INFO("Stats window created");
    return true;
}

void StatsWindow::destroy() {
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void StatsWindow::show() {
    if (hwnd_) {
        refresh();
        ShowWindow(hwnd_, SW_SHOWNORMAL);
        SetForegroundWindow(hwnd_);
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void StatsWindow::hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

bool StatsWindow::is_visible() const {
    return hwnd_ && IsWindowVisible(hwnd_);
}

void StatsWindow::refresh() {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
}

//=============================================================================
// WndProc
//=============================================================================
LRESULT CALLBACK StatsWindow::wnd_proc_static(HWND hwnd, UINT msg,
                                                WPARAM wp, LPARAM lp) {
    StatsWindow* self = nullptr;
    if (msg == WM_CREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<StatsWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<StatsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self) return self->wnd_proc(hwnd, msg, wp, lp);
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT StatsWindow::wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
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
            RECT rc;
            GetClientRect(hwnd, &rc);

            // Double-buffer
            HDC mem_dc = CreateCompatibleDC(hdc);
            HBITMAP mem_bm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HGDIOBJ old_bm = SelectObject(mem_dc, mem_bm);

            on_paint(mem_dc);

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem_dc, 0, 0, SRCCOPY);
            SelectObject(mem_dc, old_bm);
            DeleteObject(mem_bm);
            DeleteDC(mem_dc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            // Reposition export buttons
            int bw = rc.right;
            int bh = rc.bottom;
            HWND csv = GetDlgItem(hwnd, ID_EXPORT_CSV);
            HWND txt = GetDlgItem(hwnd, ID_EXPORT_TXT);
            if (csv) SetWindowPos(csv, nullptr, bw - 220, bh - 44, 90, 28, SWP_NOZORDER);
            if (txt) SetWindowPos(txt, nullptr, bw - 120, bh - 44, 90, 28, SWP_NOZORDER);
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int mx = LOWORD(lp);
            int my = HIWORD(lp);
            // Check tab click area
            RECT rc;
            GetClientRect(hwnd, &rc);
            int tab_w = rc.right / kTabCount;
            if (my >= kHeaderH && my < kHeaderH + kTabH) {
                int tab = mx / tab_w;
                if (tab >= 0 && tab < kTabCount) {
                    on_tab_click(tab);
                }
            }
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wp)) {
                case ID_EXPORT_CSV:
                case ID_EXPORT_TXT:
                    on_export();
                    break;
            }
            return 0;

        case WM_CTLCOLORBTN:
        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wp);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, theme().text_primary);
            SetBkColor(hdc, theme().bg_secondary);
            return reinterpret_cast<LRESULT>(theme().br_secondary);
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
            mmi->ptMinTrackSize = { kMinW, kMinH };
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

//=============================================================================
// Painting
//=============================================================================
void StatsWindow::on_paint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right;
    int h = rc.bottom;

    // Background
    HBRUSH bg_br = CreateSolidBrush(theme().bg_primary);
    FillRect(hdc, &rc, bg_br);
    DeleteObject(bg_br);

    draw_header(hdc, w);
    draw_tabs(hdc, w);

    // Content area
    RECT content = { 0, kHeaderH + kTabH, w, h - 50 };

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, font_normal_);

    switch (active_tab_) {
        case 0: draw_today(hdc, content);   break;
        case 1: draw_month(hdc, content);   break;
        case 2: draw_alltime(hdc, content); break;
        case 3: draw_graph(hdc, content);   break;
    }

    // Bottom separator
    HPEN sep_pen = CreatePen(PS_SOLID, 1, theme().border_subtle);
    HPEN old_pen = static_cast<HPEN>(SelectObject(hdc, sep_pen));
    MoveToEx(hdc, 0, h - 50, nullptr);
    LineTo(hdc, w, h - 50);
    SelectObject(hdc, old_pen);
    DeleteObject(sep_pen);
}

void StatsWindow::draw_header(HDC hdc, int w) {
    // Header bar
    RECT hdr = { 0, 0, w, kHeaderH };
    HBRUSH hdr_br = CreateSolidBrush(theme().bg_secondary);
    FillRect(hdc, &hdr, hdr_br);
    DeleteObject(hdr_br);

    // Title
    SelectObject(hdc, font_large_);
    SetTextColor(hdc, theme().text_primary);
    SetBkMode(hdc, TRANSPARENT);
    RECT title_rc = { 16, 8, w - 16, kHeaderH - 8 };
    DrawTextW(hdc, L"Traffic Statistics", -1, &title_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Bottom border
    HPEN pen = CreatePen(PS_SOLID, 1, theme().border_normal);
    HPEN old = static_cast<HPEN>(SelectObject(hdc, pen));
    MoveToEx(hdc, 0, kHeaderH - 1, nullptr);
    LineTo(hdc, w, kHeaderH - 1);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void StatsWindow::draw_tabs(HDC hdc, int w) {
    int tab_w = w / kTabCount;

    for (int i = 0; i < kTabCount; ++i) {
        RECT tab_rc = { i * tab_w, kHeaderH, (i + 1) * tab_w, kHeaderH + kTabH };

        bool active = (i == active_tab_);
        COLORREF bg = active ? theme().bg_primary : theme().bg_secondary;
        HBRUSH br = CreateSolidBrush(bg);
        FillRect(hdc, &tab_rc, br);
        DeleteObject(br);

        // Active indicator bar
        if (active) {
            HBRUSH acc_br = CreateSolidBrush(theme().border_accent);
            RECT ind = { tab_rc.left, tab_rc.bottom - 2,
                         tab_rc.right, tab_rc.bottom };
            FillRect(hdc, &ind, acc_br);
            DeleteObject(acc_br);
        }

        // Tab label
        SelectObject(hdc, active ? font_bold_ : font_normal_);
        SetTextColor(hdc, active ? theme().text_primary : theme().text_secondary);
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, kTabNames[i], -1, &tab_rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Right border (between tabs)
        if (i < kTabCount - 1) {
            HPEN pen = CreatePen(PS_SOLID, 1, theme().border_subtle);
            HPEN old = static_cast<HPEN>(SelectObject(hdc, pen));
            MoveToEx(hdc, (i + 1) * tab_w, kHeaderH, nullptr);
            LineTo(hdc, (i + 1) * tab_w, kHeaderH + kTabH);
            SelectObject(hdc, old);
            DeleteObject(pen);
        }
    }
}

//=============================================================================
// Tab content drawing helpers
//=============================================================================
void StatsWindow::draw_stat_row(HDC hdc, int x, int y, int row_w,
                                 const wchar_t* label,
                                 const std::wstring& value,
                                 COLORREF val_color) {
    int lw = row_w / 2;

    SetTextColor(hdc, theme().text_secondary);
    RECT lrc = { x, y, x + lw, y + 24 };
    DrawTextW(hdc, label, -1, &lrc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, val_color ? val_color : theme().text_primary);
    RECT vrc = { x + lw, y, x + row_w, y + 24 };
    DrawTextW(hdc, value.c_str(), -1, &vrc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
}

void StatsWindow::draw_limit_bar(HDC hdc, int x, int y, int w,
                                  uint64_t used, uint64_t limit) {
    if (limit == 0) return;

    double pct = std::min(static_cast<double>(used) / limit, 1.0);
    int bar_h = 8;

    // Background
    RECT bg_rc = { x, y, x + w, y + bar_h };
    HBRUSH bg_br = CreateSolidBrush(theme().bg_tertiary);
    FillRect(hdc, &bg_rc, bg_br);
    DeleteObject(bg_br);

    // Fill
    COLORREF fill_col = pct >= 1.0 ? theme().error_color
                      : pct >= 0.8 ? theme().warn_color
                      :              theme().rx_color;
    RECT fill_rc = { x, y, x + static_cast<int>(w * pct), y + bar_h };
    if (fill_rc.right > fill_rc.left) {
        HBRUSH fill_br = CreateSolidBrush(fill_col);
        FillRect(hdc, &fill_rc, fill_br);
        DeleteObject(fill_br);
    }

    // Label: "1.5 GB / 10 GB (15%)"
    wchar_t pct_buf[16];
    swprintf(pct_buf, 16, L"%.0f%%", pct * 100.0);
    std::wstring label = utils::format_bytes(used) + L" / " +
                         utils::format_bytes(limit) + L" (" + pct_buf + L")";

    RECT lrc = { x, y + bar_h + 4, x + w, y + bar_h + 20 };
    SetTextColor(hdc, theme().text_secondary);
    DrawTextW(hdc, label.c_str(), -1, &lrc, DT_CENTER | DT_TOP | DT_SINGLELINE);
}

//=============================================================================
void StatsWindow::draw_today(HDC hdc, const RECT& rc) {
    SelectObject(hdc, font_normal_);
    int x = rc.left + 20;
    int y = rc.top  + 20;
    int w = rc.right - rc.left - 40;

    // Section title
    SelectObject(hdc, font_bold_);
    SetTextColor(hdc, theme().text_primary);
    RECT title_rc = { x, y, x + w, y + 24 };
    DrawTextW(hdc, L"Today's Traffic", -1, &title_rc, DT_LEFT | DT_TOP | DT_SINGLELINE);
    y += 36;

    SelectObject(hdc, font_normal_);

    auto rx = stats_.today_rx();
    auto tx = stats_.today_tx();
    auto tot = stats_.today_total();
    auto daily_lim = stats_.daily_limit();

    draw_stat_row(hdc, x, y, w, L"Downloaded:",
                  utils::format_bytes(rx), theme().rx_color);
    y += 28;
    draw_stat_row(hdc, x, y, w, L"Uploaded:",
                  utils::format_bytes(tx), theme().tx_color);
    y += 28;

    // Separator
    HPEN sep = CreatePen(PS_SOLID, 1, theme().border_subtle);
    HPEN old = static_cast<HPEN>(SelectObject(hdc, sep));
    MoveToEx(hdc, x, y, nullptr);
    LineTo(hdc, x + w, y);
    SelectObject(hdc, old);
    DeleteObject(sep);
    y += 12;

    draw_stat_row(hdc, x, y, w, L"Total:", utils::format_bytes(tot));
    y += 40;

    // Daily limit bar
    if (daily_lim > 0) {
        SelectObject(hdc, font_bold_);
        SetTextColor(hdc, theme().text_primary);
        RECT lim_title = { x, y, x + w, y + 24 };
        DrawTextW(hdc, L"Daily Limit", -1, &lim_title, DT_LEFT | DT_TOP | DT_SINGLELINE);
        y += 28;
        draw_limit_bar(hdc, x, y, w, tot, daily_lim);
    }
}

void StatsWindow::draw_month(HDC hdc, const RECT& rc) {
    SelectObject(hdc, font_normal_);
    int x = rc.left + 20;
    int y = rc.top  + 20;
    int w = rc.right - rc.left - 40;

    SelectObject(hdc, font_bold_);
    SetTextColor(hdc, theme().text_primary);
    RECT title_rc = { x, y, x + w, y + 24 };
    DrawTextW(hdc, L"This Month", -1, &title_rc, DT_LEFT | DT_TOP | DT_SINGLELINE);
    y += 36;

    SelectObject(hdc, font_normal_);

    auto rx  = stats_.month_rx();
    auto tx  = stats_.month_tx();
    auto tot = stats_.month_total();
    auto mon_lim = stats_.monthly_limit();

    draw_stat_row(hdc, x, y, w, L"Downloaded:", utils::format_bytes(rx), theme().rx_color);
    y += 28;
    draw_stat_row(hdc, x, y, w, L"Uploaded:",   utils::format_bytes(tx), theme().tx_color);
    y += 28;

    HPEN sep = CreatePen(PS_SOLID, 1, theme().border_subtle);
    HPEN old = static_cast<HPEN>(SelectObject(hdc, sep));
    MoveToEx(hdc, x, y, nullptr); LineTo(hdc, x + w, y);
    SelectObject(hdc, old); DeleteObject(sep);
    y += 12;

    draw_stat_row(hdc, x, y, w, L"Total:", utils::format_bytes(tot));
    y += 40;

    // Day history table (last 7 days)
    SelectObject(hdc, font_bold_);
    SetTextColor(hdc, theme().text_primary);
    RECT days_title = { x, y, x + w, y + 24 };
    DrawTextW(hdc, L"Recent Days", -1, &days_title, DT_LEFT | DT_TOP | DT_SINGLELINE);
    y += 28;

    auto records = stats_.get_day_records(7);
    SelectObject(hdc, font_small_);
    for (const auto& r : records) {
        uint32_t yr = r.key / 10000;
        uint32_t mo = (r.key / 100) % 100;
        uint32_t dy = r.key % 100;
        wchar_t date_buf[16];
        swprintf(date_buf, 16, L"%04u-%02u-%02u", yr, mo, dy);
        std::wstring date = date_buf;
        std::wstring info = std::wstring(L"\u2193") + utils::format_bytes(r.rx) +
                            L" \u2191" + utils::format_bytes(r.tx);

        draw_stat_row(hdc, x, y, w, date.c_str(), info);
        y += 22;
        if (y > rc.bottom - 20) break;
    }

    if (mon_lim > 0) {
        y += 10;
        SelectObject(hdc, font_bold_);
        SetTextColor(hdc, theme().text_primary);
        RECT lim_title = { x, y, x + w, y + 24 };
        DrawTextW(hdc, L"Monthly Limit", -1, &lim_title, DT_LEFT | DT_TOP | DT_SINGLELINE);
        y += 28;
        draw_limit_bar(hdc, x, y, w, tot, mon_lim);
    }
}

void StatsWindow::draw_alltime(HDC hdc, const RECT& rc) {
    SelectObject(hdc, font_normal_);
    int x = rc.left + 20;
    int y = rc.top  + 20;
    int w = rc.right - rc.left - 40;

    SelectObject(hdc, font_bold_);
    SetTextColor(hdc, theme().text_primary);
    RECT title_rc = { x, y, x + w, y + 24 };
    DrawTextW(hdc, L"All Time", -1, &title_rc, DT_LEFT | DT_TOP | DT_SINGLELINE);
    y += 36;

    SelectObject(hdc, font_normal_);
    draw_stat_row(hdc, x, y, w, L"Total Downloaded:",
                  utils::format_bytes(stats_.alltime_rx()), theme().rx_color);
    y += 28;
    draw_stat_row(hdc, x, y, w, L"Total Uploaded:",
                  utils::format_bytes(stats_.alltime_tx()), theme().tx_color);
    y += 28;

    HPEN sep = CreatePen(PS_SOLID, 1, theme().border_subtle);
    HPEN old = static_cast<HPEN>(SelectObject(hdc, sep));
    MoveToEx(hdc, x, y, nullptr); LineTo(hdc, x + w, y);
    SelectObject(hdc, old); DeleteObject(sep);
    y += 12;

    draw_stat_row(hdc, x, y, w, L"Grand Total:",
                  utils::format_bytes(stats_.alltime_total()));
    y += 50;

    // Full history (all days)
    SelectObject(hdc, font_bold_);
    SetTextColor(hdc, theme().text_primary);
    RECT days_title = { x, y, x + w, y + 24 };
    DrawTextW(hdc, L"Day History (last 31 days)", -1, &days_title,
              DT_LEFT | DT_TOP | DT_SINGLELINE);
    y += 28;

    auto records = stats_.get_day_records(31);
    SelectObject(hdc, font_small_);
    for (const auto& r : records) {
        uint32_t yr = r.key / 10000;
        uint32_t mo = (r.key / 100) % 100;
        uint32_t dy = r.key % 100;
        wchar_t date_buf[16];
        swprintf(date_buf, 16, L"%04u-%02u-%02u", yr, mo, dy);
        std::wstring date = date_buf;
        std::wstring info = utils::format_bytes(r.total());
        draw_stat_row(hdc, x, y, w, date.c_str(), info);
        y += 20;
        if (y > rc.bottom - 20) break;
    }
}

void StatsWindow::draw_graph(HDC hdc, const RECT& rc) {
    // Speed graph (last 120 samples)
    auto samples = stats_.get_speed_history(300);

    RECT graph_rc = {
        rc.left  + 10, rc.top + 10,
        rc.right - 10, rc.bottom - 10
    };

    GraphPainter::Options opt;
    opt.bg_color   = theme().graph_bg;
    opt.grid_color = theme().graph_grid;
    opt.rx_color   = theme().rx_color;
    opt.tx_color   = theme().tx_color;
    opt.text_color = theme().text_secondary;

    graph_painter_.paint(hdc, graph_rc, samples, opt);
}

//=============================================================================
void StatsWindow::on_tab_click(int tab_index) {
    if (tab_index != active_tab_) {
        active_tab_ = tab_index;
        InvalidateRect(hwnd_, nullptr, TRUE);
    }
}

void StatsWindow::on_export() {
    wchar_t path[MAX_PATH] = {};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd_;
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrFilter = L"CSV Files\0*.csv\0Text Files\0*.txt\0All Files\0*.*\0";
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"csv";

    if (!GetSaveFileNameW(&ofn)) return;

    std::wstring file_path(path);
    auto records = stats_.get_day_records(31);

    bool ok = false;
    if (file_path.ends_with(L".txt") || file_path.ends_with(L".TXT")) {
        ok = persistence_.export_txt(records, file_path);
    } else {
        ok = persistence_.export_csv(records, file_path);
    }

    if (ok) {
        MessageBoxW(hwnd_, L"Export successful!", L"Export", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBoxW(hwnd_, L"Export failed.", L"Export", MB_OK | MB_ICONERROR);
    }
}

} // namespace ui
