#pragma once
#include <windows.h>

namespace ui {

//-----------------------------------------------------------------------------
// Dark theme color palette
//-----------------------------------------------------------------------------
struct Theme {
    // Window backgrounds
    COLORREF bg_primary    { RGB(18,  18,  24)  }; // Darkest bg
    COLORREF bg_secondary  { RGB(28,  28,  36)  }; // Panel bg
    COLORREF bg_tertiary   { RGB(38,  38,  50)  }; // Card / row bg
    COLORREF bg_hover      { RGB(48,  48,  62)  }; // Hover state
    COLORREF bg_selected   { RGB(42,  82, 152)  }; // Selection

    // Borders
    COLORREF border_subtle { RGB(50,  50,  65)  };
    COLORREF border_normal { RGB(70,  70,  90)  };
    COLORREF border_accent { RGB(80, 130, 210)  };

    // Text
    COLORREF text_primary  { RGB(230, 230, 240) };
    COLORREF text_secondary{ RGB(160, 160, 180) };
    COLORREF text_disabled { RGB( 90,  90, 110) };
    COLORREF text_accent   { RGB(100, 180, 255) };

    // Traffic colors
    COLORREF rx_color      { RGB( 80, 160, 255) }; // Download — blue
    COLORREF tx_color      { RGB( 80, 220, 120) }; // Upload   — green

    // Alert colors
    COLORREF warn_color    { RGB(255, 190,  50) }; // 80% limit
    COLORREF error_color   { RGB(255,  70,  70) }; // 100% limit
    COLORREF ok_color      { RGB( 80, 220, 120) };

    // Graph
    COLORREF graph_bg      { RGB(22,  22,  30)  };
    COLORREF graph_grid    { RGB(40,  40,  55)  };

    // Overlay text colors by user setting
    COLORREF overlay_red   { RGB(255,  80,  80) };
    COLORREF overlay_green { RGB( 80, 220,  80) };
    COLORREF overlay_blue  { RGB( 80, 180, 255) };
    COLORREF overlay_white { RGB(230, 230, 240) };
    COLORREF overlay_yellow{ RGB(255, 220,  50) };

    // GDI brushes (cached, created on demand)
    HBRUSH  br_primary{ nullptr };
    HBRUSH  br_secondary{ nullptr };
    HBRUSH  br_tertiary{ nullptr };

    void create_brushes() {
        destroy_brushes();
        br_primary   = CreateSolidBrush(bg_primary);
        br_secondary = CreateSolidBrush(bg_secondary);
        br_tertiary  = CreateSolidBrush(bg_tertiary);
    }

    void destroy_brushes() {
        if (br_primary)   { DeleteObject(br_primary);   br_primary   = nullptr; }
        if (br_secondary) { DeleteObject(br_secondary); br_secondary = nullptr; }
        if (br_tertiary)  { DeleteObject(br_tertiary);  br_tertiary  = nullptr; }
    }

    ~Theme() { destroy_brushes(); }
};

//-----------------------------------------------------------------------------
// Global theme accessor
//-----------------------------------------------------------------------------
inline Theme& theme() {
    static Theme t;
    return t;
}

//-----------------------------------------------------------------------------
// Helper: create a LOGFONT for the UI
//-----------------------------------------------------------------------------
inline HFONT create_font(int height, bool bold = false,
                          const wchar_t* face = L"Segoe UI") {
    LOGFONTW lf{};
    lf.lfHeight         = -height; // Negative = character height in pixels
    lf.lfWeight         = bold ? FW_SEMIBOLD : FW_NORMAL;
    lf.lfQuality        = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
    wcscpy_s(lf.lfFaceName, face);
    return CreateFontIndirectW(&lf);
}

//-----------------------------------------------------------------------------
// Helper: create monospace font for overlay
//-----------------------------------------------------------------------------
inline HFONT create_mono_font(int height, bool bold = false) {
    LOGFONTW lf{};
    lf.lfHeight         = -height;
    lf.lfWeight         = bold ? FW_BOLD : FW_NORMAL;
    lf.lfQuality        = NONANTIALIASED_QUALITY; // Required for layered window alpha
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    wcscpy_s(lf.lfFaceName, L"Consolas");
    return CreateFontIndirectW(&lf);
}

} // namespace ui
