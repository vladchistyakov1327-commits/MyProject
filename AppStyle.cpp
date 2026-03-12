#include "AppStyle.h"
using namespace AppPalette;

// Заменяет именованные токены на реальные hex-значения
static QString S(const char* token) { return QString(token); }

QString appStyleSheet()
{
    return QString(R"(
/* ══════════════════════════════════════════════════════════
   NestingApp — Industrial Dark Stylesheet
   ══════════════════════════════════════════════════════════ */

/* ── QMainWindow / QWidget ──────────────────────────────── */
QMainWindow, QDialog {
    background: %BG_BASE%;
    color: %TEXT_BRIGHT%;
}
QWidget {
    background: %BG_BASE%;
    color: %TEXT_NORMAL%;
    font-family: "Segoe UI", "Arial", sans-serif;
    font-size: 13px;
}

/* ── QTabWidget / QTabBar ───────────────────────────────── */
QTabWidget::pane {
    border: 1px solid %BORDER_MID%;
    background: %BG_SURFACE%;
    border-radius: 2px;
}
QTabBar::tab {
    background: %BG_OVERLAY%;
    color: %TEXT_DIM%;
    padding: 8px 18px;
    border: 1px solid %BORDER_DARK%;
    border-bottom: 2px solid transparent;
    margin-right: 2px;
    border-radius: 2px 2px 0 0;
}
QTabBar::tab:selected {
    background: %BG_SURFACE%;
    color: %TEXT_BRIGHT%;
    border-bottom: 2px solid %BLUE_BORDER%;
}
QTabBar::tab:hover:!selected {
    color: %TEXT_NORMAL%;
    background: %BG_RAISED%;
}

/* ── QPushButton — base ─────────────────────────────────── */
QPushButton {
    background: %BG_RAISED%;
    color: %TEXT_NORMAL%;
    border: 1px solid %BORDER_MID%;
    padding: 6px 16px;
    border-radius: 4px;
    min-height: 28px;
}
QPushButton:hover  { background: %BG_OVERLAY%; color: %TEXT_BRIGHT%; border-color: %BORDER_LIGHT%; }
QPushButton:pressed{ background: %BG_DEEP%;    color: %TEXT_BRIGHT%; }
QPushButton:disabled{ color: %TEXT_GHOST%; border-color: %BORDER_DARK%; }

/* ── QPushButton[accent="green"] — Рассчитать ──────────── */
QPushButton[accent="green"] {
    background: %GREEN_DARK%;
    color: %GREEN_TEXT%;
    border: 1px solid %GREEN_BORDER%;
    font-weight: bold;
}
QPushButton[accent="green"]:hover   { background: %GREEN_MID%; }
QPushButton[accent="green"]:pressed { background: %GREEN_DIM%; }
QPushButton[accent="green"]:disabled{ background: %BG_RAISED%; color: %TEXT_GHOST%; border-color: %BORDER_DARK%; }

/* ── QPushButton[accent="blue"] — Импорт, Экспорт ──────── */
QPushButton[accent="blue"] {
    background: %BLUE_DARK%;
    color: %BLUE_TEXT%;
    border: 1px solid %BLUE_BORDER%;
}
QPushButton[accent="blue"]:hover   { background: %BLUE_MID%; }
QPushButton[accent="blue"]:pressed { background: %BLUE_DARK%; border-color: %BLUE_BRIGHT%; }

/* ── QPushButton[accent="red"] — Стоп, Удалить ─────────── */
QPushButton[accent="red"] {
    background: %RED_DARK%;
    color: %RED_TEXT%;
    border: 1px solid %RED_BORDER%;
}
QPushButton[accent="red"]:hover   { background: %RED_MID%; }
QPushButton[accent="red"]:pressed { background: %RED_DIM%; }

/* ── QTableView ─────────────────────────────────────────── */
QTableView {
    background: %BG_DEEP%;
    color: %TEXT_NORMAL%;
    gridline-color: %BORDER_DARK%;
    border: 1px solid %BORDER_MID%;
    selection-background-color: %BLUE_MID%;
    selection-color: %TEXT_WHITE%;
    alternate-background-color: %BG_SURFACE%;
}
QTableView::item:hover { background: %BG_OVERLAY%; }
QHeaderView::section {
    background: %BG_OVERLAY%;
    color: %TEXT_DIM%;
    border: none;
    border-bottom: 1px solid %BORDER_MID%;
    border-right: 1px solid %BORDER_DARK%;
    padding: 5px 8px;
    font-weight: bold;
}
QHeaderView::section:hover { background: %BG_RAISED%; color: %TEXT_NORMAL%; }

/* ── QDoubleSpinBox / QSpinBox ──────────────────────────── */
QDoubleSpinBox, QSpinBox {
    background: %BG_RAISED%;
    color: %TEXT_BRIGHT%;
    border: 1px solid %BORDER_MID%;
    padding: 4px 8px;
    border-radius: 3px;
    min-height: 24px;
}
QDoubleSpinBox:focus, QSpinBox:focus {
    border: 1px solid %BORDER_LIGHT%;
    background: %BG_SURFACE%;
}
QDoubleSpinBox::up-button, QSpinBox::up-button,
QDoubleSpinBox::down-button, QSpinBox::down-button {
    background: %BG_OVERLAY%;
    border: none;
    width: 18px;
}
QDoubleSpinBox::up-button:hover, QSpinBox::up-button:hover,
QDoubleSpinBox::down-button:hover, QSpinBox::down-button:hover {
    background: %BORDER_MID%;
}

/* ── QLineEdit ──────────────────────────────────────────── */
QLineEdit {
    background: %BG_RAISED%;
    color: %TEXT_BRIGHT%;
    border: 1px solid %BORDER_MID%;
    padding: 4px 8px;
    border-radius: 3px;
    min-height: 24px;
}
QLineEdit:focus  { border: 1px solid %BORDER_LIGHT%; background: %BG_SURFACE%; }
QLineEdit:read-only { color: %TEXT_DIM%; background: %BG_OVERLAY%; }

/* ── QComboBox ──────────────────────────────────────────── */
QComboBox {
    background: %BG_RAISED%;
    color: %TEXT_BRIGHT%;
    border: 1px solid %BORDER_MID%;
    padding: 4px 8px;
    border-radius: 3px;
    min-height: 24px;
}
QComboBox:focus { border-color: %BORDER_LIGHT%; }
QComboBox::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView {
    background: %BG_OVERLAY%;
    color: %TEXT_NORMAL%;
    border: 1px solid %BORDER_MID%;
    selection-background-color: %BLUE_MID%;
    selection-color: %TEXT_WHITE%;
}

/* ── QGroupBox ──────────────────────────────────────────── */
QGroupBox {
    color: %TEXT_DIM%;
    border: 1px solid %BORDER_DARK%;
    border-radius: 4px;
    margin-top: 12px;
    padding-top: 18px;
    padding-left: 8px;
    font-size: 12px;
}
QGroupBox::title {
    color: %CYAN_TEXT%;
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
    left: 8px;
    font-weight: bold;
    font-size: 11px;
    letter-spacing: 0.5px;
}

/* ── QCheckBox ──────────────────────────────────────────── */
QCheckBox { color: %TEXT_NORMAL%; spacing: 6px; }
QCheckBox::indicator {
    width: 14px; height: 14px;
    background: %BG_RAISED%;
    border: 1px solid %BORDER_MID%;
    border-radius: 2px;
}
QCheckBox::indicator:checked {
    background: %BLUE_MID%;
    border-color: %BLUE_BORDER%;
}
QCheckBox::indicator:hover { border-color: %BORDER_LIGHT%; }

/* ── QRadioButton ───────────────────────────────────────── */
QRadioButton { color: %TEXT_NORMAL%; spacing: 6px; }
QRadioButton::indicator {
    width: 14px; height: 14px;
    background: %BG_RAISED%;
    border: 1px solid %BORDER_MID%;
    border-radius: 7px;
}
QRadioButton::indicator:checked {
    background: %BLUE_BRIGHT%;
    border-color: %BLUE_BORDER%;
}

/* ── QSlider ────────────────────────────────────────────── */
QSlider::groove:horizontal {
    height: 4px;
    background: %BG_OVERLAY%;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: %BLUE_BRIGHT%;
    border: 1px solid %BLUE_BORDER%;
    width: 14px; height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
QSlider::sub-page:horizontal { background: %BLUE_MID%; border-radius: 2px; }

/* ── QScrollBar ─────────────────────────────────────────── */
QScrollBar:vertical {
    background: %BG_DEEP%;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: %BORDER_MID%;
    border-radius: 4px;
    min-height: 20px;
}
QScrollBar::handle:vertical:hover { background: %BORDER_LIGHT%; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QScrollBar:horizontal {
    background: %BG_DEEP%;
    height: 8px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: %BORDER_MID%;
    border-radius: 4px;
    min-width: 20px;
}
QScrollBar::handle:horizontal:hover { background: %BORDER_LIGHT%; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── QSplitter ──────────────────────────────────────────── */
QSplitter::handle { background: %BORDER_DARK%; }
QSplitter::handle:hover { background: %BORDER_MID%; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical   { height: 1px; }

/* ── QLabel ─────────────────────────────────────────────── */
QLabel { color: %TEXT_NORMAL%; background: transparent; }
QLabel[role="title"]    { color: %TEXT_BRIGHT%; font-weight: bold; font-size: 14px; }
QLabel[role="subtitle"] { color: %TEXT_DIM%; font-size: 11px; }
QLabel[role="error"]    { color: %RED_TEXT%; }
QLabel[role="warn"]     { color: %AMBER_TEXT%; }
QLabel[role="ok"]       { color: %GREEN_TEXT%; }

/* ── QToolBar / TopBar ──────────────────────────────────── */
#topBar {
    background: %TOPBAR_BG%;
    border-bottom: 1px solid %TOPBAR_BORD%;
    padding: 4px 12px;
    spacing: 8px;
}
QToolBar {
    background: %BG_OVERLAY%;
    border: none;
    spacing: 4px;
}
QToolBar::separator {
    background: %BORDER_DARK%;
    width: 1px;
    margin: 4px 6px;
}

/* ── QStatusBar ─────────────────────────────────────────── */
QStatusBar {
    background: %TOPBAR_BG%;
    color: %TEXT_DIM%;
    border-top: 1px solid %TOPBAR_BORD%;
    font-size: 12px;
}
QStatusBar::item { border: none; }

/* ── QMenuBar ───────────────────────────────────────────── */
QMenuBar {
    background: %BG_OVERLAY%;
    color: %TEXT_NORMAL%;
    border-bottom: 1px solid %BORDER_DARK%;
    padding: 2px 4px;
}
QMenuBar::item:selected { background: %BG_RAISED%; color: %TEXT_BRIGHT%; border-radius: 3px; }
QMenu {
    background: %BG_OVERLAY%;
    color: %TEXT_NORMAL%;
    border: 1px solid %BORDER_MID%;
    padding: 4px;
}
QMenu::item:selected { background: %BLUE_MID%; color: %TEXT_WHITE%; border-radius: 2px; }
QMenu::separator { background: %BORDER_DARK%; height: 1px; margin: 4px 8px; }

/* ── QScrollArea ────────────────────────────────────────── */
QScrollArea { border: none; background: transparent; }
QScrollArea > QWidget > QWidget { background: transparent; }

/* ── QProgressBar ───────────────────────────────────────── */
QProgressBar {
    background: %BG_RAISED%;
    border: 1px solid %BORDER_MID%;
    border-radius: 3px;
    color: %TEXT_BRIGHT%;
    text-align: center;
    font-size: 12px;
    min-height: 18px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 %BLUE_MID%, stop:1 %BLUE_BRIGHT%);
    border-radius: 2px;
}

/* ── QFrame (separator) ─────────────────────────────────── */
QFrame[frameShape="4"], QFrame[frameShape="5"] {
    color: %BORDER_DARK%;
}

/* ── QToolTip ───────────────────────────────────────────── */
QToolTip {
    background: %BG_OVERLAY%;
    color: %TEXT_BRIGHT%;
    border: 1px solid %BORDER_MID%;
    padding: 4px 8px;
    border-radius: 3px;
}
)")
    .replace("%BG_VOID%",    S(BG_VOID)   ).replace("%BG_BASE%",    S(BG_BASE)   )
    .replace("%BG_DEEP%",    S(BG_DEEP)   ).replace("%BG_SURFACE%", S(BG_SURFACE))
    .replace("%BG_OVERLAY%", S(BG_OVERLAY)).replace("%BG_RAISED%",  S(BG_RAISED) )
    .replace("%TEXT_WHITE%",  S(TEXT_WHITE) ).replace("%TEXT_BRIGHT%", S(TEXT_BRIGHT))
    .replace("%TEXT_NORMAL%", S(TEXT_NORMAL)).replace("%TEXT_DIM%",    S(TEXT_DIM)   )
    .replace("%TEXT_MUTED%",  S(TEXT_MUTED) ).replace("%TEXT_GHOST%",  S(TEXT_GHOST) )
    .replace("%BORDER_DARK%",  S(BORDER_DARK) ).replace("%BORDER_MID%",   S(BORDER_MID)  )
    .replace("%BORDER_LIGHT%", S(BORDER_LIGHT))
    .replace("%BLUE_DARK%",   S(BLUE_DARK)  ).replace("%BLUE_MID%",    S(BLUE_MID)   )
    .replace("%BLUE_BRIGHT%", S(BLUE_BRIGHT)).replace("%BLUE_BORDER%", S(BLUE_BORDER))
    .replace("%BLUE_TEXT%",   S(BLUE_TEXT)  )
    .replace("%GREEN_DARK%",   S(GREEN_DARK)  ).replace("%GREEN_MID%",    S(GREEN_MID)   )
    .replace("%GREEN_DIM%",    S(GREEN_DIM)   ).replace("%GREEN_BORDER%", S(GREEN_BORDER))
    .replace("%GREEN_TEXT%",   S(GREEN_TEXT)  )
    .replace("%RED_DARK%",   S(RED_DARK)  ).replace("%RED_MID%",    S(RED_MID)   )
    .replace("%RED_DIM%",    S(RED_DIM)   ).replace("%RED_BORDER%", S(RED_BORDER))
    .replace("%RED_TEXT%",   S(RED_TEXT)  )
    .replace("%AMBER_TEXT%", S(AMBER_TEXT))
    .replace("%CYAN_TEXT%",  S(CYAN_TEXT) )
    .replace("%TOPBAR_BG%",   S(TOPBAR_BG)  )
    .replace("%TOPBAR_BORD%", S(TOPBAR_BORD));
}
