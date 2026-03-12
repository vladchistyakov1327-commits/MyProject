#pragma once
#include <QString>

// ============================================================
// NestingApp — Industrial Dark Theme Palette
// Все цвета определены здесь. НЕ ИЗМЕНЯТЬ значения.
// ============================================================
namespace AppPalette
{
    // ── Backgrounds ─────────────────────────────────────────
    static constexpr const char* BG_VOID    = "#070a0e";  // Тёмнейший фон (холст, пустота)
    static constexpr const char* BG_BASE    = "#0d1117";  // Базовый фон приложения
    static constexpr const char* BG_DEEP    = "#010409";  // Самые тёмные области
    static constexpr const char* BG_SURFACE = "#161b22";  // Поверхности панелей
    static constexpr const char* BG_OVERLAY = "#1c2128";  // Оверлеи, тулбары
    static constexpr const char* BG_RAISED  = "#22272e";  // Приподнятые элементы

    // ── Text ────────────────────────────────────────────────
    static constexpr const char* TEXT_WHITE  = "#ffffff";
    static constexpr const char* TEXT_BRIGHT = "#cdd9e5";  // Основной текст
    static constexpr const char* TEXT_NORMAL = "#adbac7";  // Обычный текст
    static constexpr const char* TEXT_DIM    = "#768390";  // Метки, подписи
    static constexpr const char* TEXT_MUTED  = "#545d68";  // Подсказки, placeholder
    static constexpr const char* TEXT_GHOST  = "#373e47";  // Еле видимые элементы

    // ── Borders ─────────────────────────────────────────────
    static constexpr const char* BORDER_DARK  = "#1e2228"; // Тонкий разделитель
    static constexpr const char* BORDER_MID   = "#343b45"; // Обычная граница
    static constexpr const char* BORDER_LIGHT = "#4b535e"; // Активная граница

    // ── Blue (primary actions) ──────────────────────────────
    static constexpr const char* BLUE_DARK   = "#0d2045";
    static constexpr const char* BLUE_MID    = "#1c4a8a";
    static constexpr const char* BLUE_BRIGHT = "#316dca";
    static constexpr const char* BLUE_BORDER = "#2f81f7";
    static constexpr const char* BLUE_TEXT   = "#79acff";

    // ── Green (start, success) ──────────────────────────────
    static constexpr const char* GREEN_DARK   = "#0a2010";
    static constexpr const char* GREEN_MID    = "#196c2e";
    static constexpr const char* GREEN_DIM    = "#122d1c";
    static constexpr const char* GREEN_BORDER = "#238636";
    static constexpr const char* GREEN_TEXT   = "#3fb950";

    // ── Red (cancel, error, delete) ─────────────────────────
    static constexpr const char* RED_DARK   = "#2a0d0d";
    static constexpr const char* RED_MID    = "#6b1a1a";
    static constexpr const char* RED_DIM    = "#3d0f0f";
    static constexpr const char* RED_BORDER = "#da3633";
    static constexpr const char* RED_TEXT   = "#f85149";

    // ── Amber (warnings) ────────────────────────────────────
    static constexpr const char* AMBER_TEXT = "#d29922";

    // ── Cyan (section headings, group titles) ───────────────
    static constexpr const char* CYAN_TEXT  = "#56d2e0";

    // ── TopBar ──────────────────────────────────────────────
    static constexpr const char* TOPBAR_BG   = "#0d1117";
    static constexpr const char* TOPBAR_BORD = "#21262d";

    // ── Aliases (convenience) ───────────────────────────────
    static constexpr const char* BG_PANEL      = BG_SURFACE;  ///< Фон карточки/панели
    static constexpr const char* BORDER_SUBTLE = BORDER_DARK; ///< Тонкая граница карточки
}

/// Возвращает полный QSS stylesheet приложения.
QString appStyleSheet();
