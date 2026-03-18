#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../core/statistics.h"
#include "../core/persistence.h"
#include "graph_painter.h"

namespace ui {

//-----------------------------------------------------------------------------
// StatsWindow — floating statistics window with tabs:
//   [Today] [Month] [All Time] [Graph]
//-----------------------------------------------------------------------------
class StatsWindow {
public:
    explicit StatsWindow(core::Statistics& stats, core::Persistence& persistence)
        : stats_(stats), persistence_(persistence) {}
    ~StatsWindow();

    bool create(HINSTANCE hInst, HWND parent = nullptr);
    void destroy();
    void show();
    void hide();
    bool is_visible() const;

    void refresh(); // Re-query stats and repaint

private:
    static LRESULT CALLBACK wnd_proc_static(HWND, UINT, WPARAM, LPARAM);
    LRESULT wnd_proc(HWND, UINT, WPARAM, LPARAM);

    void on_paint(HDC hdc);
    void on_size(int w, int h);
    void on_tab_click(int tab_index);
    void on_export();

    void draw_header(HDC hdc, int w);
    void draw_tabs(HDC hdc, int w);
    void draw_today(HDC hdc, const RECT& rc);
    void draw_month(HDC hdc, const RECT& rc);
    void draw_alltime(HDC hdc, const RECT& rc);
    void draw_graph(HDC hdc, const RECT& rc);

    // Helpers
    void draw_stat_row(HDC hdc, int x, int y, int row_w,
                       const wchar_t* label, const std::wstring& value,
                       COLORREF val_color = 0);
    void draw_limit_bar(HDC hdc, int x, int y, int w,
                        uint64_t used, uint64_t limit);

    static constexpr wchar_t kClassName[] = L"TrafficStats_v3";
    static constexpr int kTabCount  = 4;
    static constexpr int kHeaderH   = 50;
    static constexpr int kTabH      = 36;
    static constexpr int kMinW      = 420;
    static constexpr int kMinH      = 380;

    HWND             hwnd_{ nullptr };
    HINSTANCE        hInst_{ nullptr };
    bool             registered_{ false };
    int              active_tab_{ 0 };

    HFONT            font_normal_{ nullptr };
    HFONT            font_bold_{ nullptr };
    HFONT            font_small_{ nullptr };
    HFONT            font_large_{ nullptr };

    GraphPainter     graph_painter_;

    core::Statistics&    stats_;
    core::Persistence&   persistence_;

    // Button IDs
    enum { ID_EXPORT_CSV = 101, ID_EXPORT_TXT = 102, ID_CLOSE = 103 };
};

} // namespace ui
