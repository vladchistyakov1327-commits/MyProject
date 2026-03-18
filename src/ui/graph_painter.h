#pragma once
#include <windows.h>
#include <vector>
#include "../core/statistics.h"
#include "theme.h"

namespace ui {

//-----------------------------------------------------------------------------
// GraphPainter — renders a speed history graph into a GDI DC
// Uses double-buffering, no GDI+ dependency (pure GDI polyline)
//-----------------------------------------------------------------------------
class GraphPainter {
public:
    struct Options {
        COLORREF bg_color   { RGB(22, 22, 30) };
        COLORREF grid_color { RGB(40, 40, 55) };
        COLORREF rx_color   { RGB(80, 160, 255) };
        COLORREF tx_color   { RGB(80, 220, 120) };
        COLORREF text_color { RGB(160, 160, 180) };
        int      grid_lines { 4 };
        bool     show_legend{ true };
        bool     filled     { true }; // Fill under curve
    };

    GraphPainter() = default;

    // Paint the graph into the given DC rectangle.
    // samples: speed samples ordered oldest->newest
    void paint(HDC hdc, const RECT& rc,
               const std::vector<core::SpeedSample>& samples,
               const Options& opt = {}) const;

    // Paint a bar chart (for daily data)
    void paint_bars(HDC hdc, const RECT& rc,
                    const std::vector<core::DayRecord>& records,
                    const Options& opt = {}) const;

private:
    void draw_grid(HDC hdc, const RECT& rc, double max_val,
                   const Options& opt) const;

    void draw_curve(HDC hdc, const RECT& rc,
                    const std::vector<core::SpeedSample>& samples,
                    double max_val, bool is_rx,
                    const Options& opt) const;

    void draw_legend(HDC hdc, const RECT& rc,
                     double cur_rx, double cur_tx,
                     const Options& opt) const;

    static double compute_max(const std::vector<core::SpeedSample>& s);
};

} // namespace ui
