#include "graph_painter.h"
#include "../utils/format_helpers.h"
#include <algorithm>
#include <cmath>

namespace ui {

//-----------------------------------------------------------------------------
void GraphPainter::paint(HDC hdc, const RECT& rc,
                          const std::vector<core::SpeedSample>& samples,
                          const Options& opt) const {
    int w = rc.right  - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    // Fill background
    HBRUSH bg_br = CreateSolidBrush(opt.bg_color);
    FillRect(hdc, &rc, bg_br);
    DeleteObject(bg_br);

    if (samples.empty()) return;

    double max_val = compute_max(samples);
    if (max_val < 1024.0) max_val = 1024.0; // Minimum 1 KB/s scale

    // Round max_val up to a "nice" number
    double magnitude = std::pow(10.0, std::floor(std::log10(max_val)));
    max_val = std::ceil(max_val / magnitude) * magnitude;

    draw_grid(hdc, rc, max_val, opt);
    draw_curve(hdc, rc, samples, max_val, false, opt); // TX first (behind)
    draw_curve(hdc, rc, samples, max_val, true,  opt); // RX on top

    // Current values
    const auto& last = samples.back();
    if (opt.show_legend) {
        draw_legend(hdc, rc, last.rx, last.tx, opt);
    }
}

//-----------------------------------------------------------------------------
void GraphPainter::paint_bars(HDC hdc, const RECT& rc,
                               const std::vector<core::DayRecord>& records,
                               const Options& opt) const {
    int w = rc.right  - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0 || records.empty()) return;

    HBRUSH bg_br = CreateSolidBrush(opt.bg_color);
    FillRect(hdc, &rc, bg_br);
    DeleteObject(bg_br);

    // Compute max total
    uint64_t max_total = 0;
    for (const auto& r : records) max_total = std::max(max_total, r.total());
    if (max_total == 0) return;

    // Draw records newest-first, so limit to last 14 or fit in width
    int n = static_cast<int>(records.size());
    n = std::min(n, (w - 20) / 12); // ~12px per bar group
    if (n <= 0) return;

    int bar_group_w = (w - 20) / n;
    int bar_pad     = 2;
    int bar_w       = (bar_group_w - bar_pad * 3) / 2;
    if (bar_w < 1) bar_w = 1;

    // We want records[0] = newest on the right
    for (int i = 0; i < n; ++i) {
        // i=0 is newest, draw on right
        const auto& r = records[i];
        int x_right = rc.right - 10 - i * bar_group_w;
        int x_rx = x_right - bar_w - bar_pad;
        int x_tx = x_right - bar_pad;

        // RX bar (blue)
        int h_rx = static_cast<int>(static_cast<double>(r.rx) / max_total * (h - 20));
        RECT bar_rc_rx = { x_rx - bar_w, rc.bottom - 15 - h_rx, x_rx, rc.bottom - 15 };
        HBRUSH rx_br = CreateSolidBrush(opt.rx_color);
        FillRect(hdc, &bar_rc_rx, rx_br);
        DeleteObject(rx_br);

        // TX bar (green)
        int h_tx = static_cast<int>(static_cast<double>(r.tx) / max_total * (h - 20));
        RECT bar_rc_tx = { x_tx - bar_w, rc.bottom - 15 - h_tx, x_tx, rc.bottom - 15 };
        HBRUSH tx_br = CreateSolidBrush(opt.tx_color);
        FillRect(hdc, &bar_rc_tx, tx_br);
        DeleteObject(tx_br);
    }
}

//-----------------------------------------------------------------------------
void GraphPainter::draw_grid(HDC hdc, const RECT& rc, double max_val,
                              const Options& opt) const {
    int w = rc.right  - rc.left;
    int h = rc.bottom - rc.top;

    HPEN grid_pen = CreatePen(PS_SOLID, 1, opt.grid_color);
    HPEN old_pen  = static_cast<HPEN>(SelectObject(hdc, grid_pen));

    int lines = opt.grid_lines;
    for (int i = 1; i <= lines; ++i) {
        int y = rc.top + (h - 20) * (lines - i) / lines + 5;
        MoveToEx(hdc, rc.left + 5,  y, nullptr);
        LineTo(hdc,   rc.right - 5, y);

        // Label
        double val = max_val * i / lines;
        std::wstring label = utils::format_speed(val);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, opt.text_color);
        RECT label_rc = { rc.left, y - 8, rc.left + 50, y + 8 };
        DrawTextW(hdc, label.c_str(), -1, &label_rc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    // Bottom border
    MoveToEx(hdc, rc.left + 5,  rc.bottom - 15, nullptr);
    LineTo(hdc,   rc.right - 5, rc.bottom - 15);

    SelectObject(hdc, old_pen);
    DeleteObject(grid_pen);
}

//-----------------------------------------------------------------------------
void GraphPainter::draw_curve(HDC hdc, const RECT& rc,
                               const std::vector<core::SpeedSample>& samples,
                               double max_val, bool is_rx,
                               const Options& opt) const {
    int n = static_cast<int>(samples.size());
    if (n < 2) return;

    int w = rc.right  - rc.left - 10;
    int h = rc.bottom - rc.top  - 20;
    int x0 = rc.left  + 5;
    int y0 = rc.bottom - 15;

    COLORREF color = is_rx ? opt.rx_color : opt.tx_color;

    // Build points
    std::vector<POINT> pts(n);
    for (int i = 0; i < n; ++i) {
        double val = is_rx ? samples[i].rx : samples[i].tx;
        int px = x0 + static_cast<int>(static_cast<double>(i) / (n - 1) * w);
        int py = y0 - static_cast<int>(std::min(val / max_val, 1.0) * h);
        pts[i] = { px, py };
    }

    if (opt.filled) {
        // Create a closed polygon for the fill
        std::vector<POINT> poly;
        poly.reserve(n + 2);
        poly.push_back({ x0, y0 });
        for (auto& p : pts) poly.push_back(p);
        poly.push_back({ x0 + w, y0 });

        // Semi-transparent fill using SRCPAINT trick — use a lighter color
        COLORREF fill_col = RGB(
            (GetRValue(color) + 18 * 2) / 3,
            (GetGValue(color) + 18 * 2) / 3,
            (GetBValue(color) + 18 * 2) / 3);
        HBRUSH fill_br = CreateSolidBrush(fill_col);
        HPEN   null_pen = CreatePen(PS_NULL, 0, 0);
        HBRUSH old_br = static_cast<HBRUSH>(SelectObject(hdc, fill_br));
        HPEN   old_pe = static_cast<HPEN>(SelectObject(hdc, null_pen));
        Polygon(hdc, poly.data(), static_cast<int>(poly.size()));
        SelectObject(hdc, old_br);
        SelectObject(hdc, old_pe);
        DeleteObject(fill_br);
        DeleteObject(null_pen);
    }

    // Draw the line on top
    HPEN line_pen = CreatePen(PS_SOLID, 2, color);
    HPEN old_pen  = static_cast<HPEN>(SelectObject(hdc, line_pen));
    Polyline(hdc, pts.data(), n);
    SelectObject(hdc, old_pen);
    DeleteObject(line_pen);
}

//-----------------------------------------------------------------------------
void GraphPainter::draw_legend(HDC hdc, const RECT& rc,
                                double cur_rx, double cur_tx,
                                const Options& opt) const {
    int x = rc.right - 5;
    int y = rc.top + 8;

    std::wstring rx_label = L"\u2193 " + utils::format_speed(cur_rx);
    std::wstring tx_label = L"\u2191 " + utils::format_speed(cur_tx);

    SetBkMode(hdc, TRANSPARENT);

    // RX label (right-aligned)
    RECT r1 = { rc.left, y, x, y + 16 };
    SetTextColor(hdc, opt.rx_color);
    DrawTextW(hdc, rx_label.c_str(), -1, &r1, DT_RIGHT | DT_TOP | DT_SINGLELINE);

    // TX label
    RECT r2 = { rc.left, y + 18, x, y + 34 };
    SetTextColor(hdc, opt.tx_color);
    DrawTextW(hdc, tx_label.c_str(), -1, &r2, DT_RIGHT | DT_TOP | DT_SINGLELINE);
}

//-----------------------------------------------------------------------------
double GraphPainter::compute_max(const std::vector<core::SpeedSample>& s) {
    double m = 0.0;
    for (const auto& p : s) {
        m = std::max(m, p.rx);
        m = std::max(m, p.tx);
    }
    return m;
}

} // namespace ui
