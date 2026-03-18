# Traffic Monitor v3.0 — Architecture Reference

## Quick Build

### With MSVC (Visual Studio 2022)
```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### With MinGW-w64
```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output binary: `bin/TrafficMonitor.exe`

---

## Project Structure

```
traffic-monitor/
├── src/
│   ├── core/
│   │   ├── data_collector.h/cpp    — GetIfTable2-based traffic collection
│   │   ├── statistics.h/cpp        — Per-day stats, speed ring buffer
│   │   ├── persistence.h/cpp       — Binary stats file + INI settings
│   │   └── traffic_monitor.h/cpp  — Main app class (tray, hotkeys, threading)
│   ├── ui/
│   │   ├── overlay_window.h/cpp   — Layered window overlay (UpdateLayeredWindow)
│   │   ├── stats_window.h/cpp     — Statistics window with 4 tabs
│   │   ├── settings_window.h/cpp  — Settings dialog
│   │   ├── graph_painter.h/cpp    — Pure GDI speed graph renderer
│   │   └── theme.h                — Dark theme color constants + font helpers
│   ├── utils/
│   │   ├── logger.h/cpp           — Thread-safe logger (OutputDebugString + file)
│   │   ├── win_helpers.h          — RAII WinAPI wrappers (handles, DCs, DIBs)
│   │   └── format_helpers.h       — Bytes/speed formatting helpers
│   └── main.cpp                   — wWinMain entry point
├── resources/
│   ├── resource.h                 — Resource IDs
│   ├── resources.rc               — Version info + icons
│   └── app.manifest               — DPI awareness + visual styles
├── CMakeLists.txt                 — Modern CMake (MSVC + MinGW)
└── OVERLAY_PROMPT.md              — This file
```

---

## Hotkeys

| Shortcut        | Action                    |
|-----------------|---------------------------|
| Ctrl+Shift+H    | Toggle overlay visibility |
| Ctrl+Shift+S    | Open Statistics window    |
| Ctrl+Shift+P    | Open Settings window      |
| Ctrl+Shift+Q    | Quit application          |

---

## Data Storage

All data is stored in `%APPDATA%\TrafficMonitor\`:

| File           | Format          | Contents                      |
|----------------|-----------------|-------------------------------|
| `stats.dat`    | Binary (custom) | Per-day RX/TX records (31 days) |
| `settings.ini` | Key=Value text  | All application settings      |
| `traffic.log`  | Text            | Debug/info log output         |

---

## Overlay Design

- `WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE`
- Rendered into a 32-bpp DIB section via `UpdateLayeredWindow`
- `NONANTIALIASED_QUALITY` font for correct per-pixel alpha
- Drop shadow: text drawn twice (offset 2px in black, then in color)
- Font: **Consolas** (monospace, ensures fixed-width alignment)
- ↓ blue (RGB 80,160,255) for download speed
- ↑ green (RGB 80,220,120) for upload speed
- 8 screen positions + configurable X/Y offset

---

## Threading Model

```
Main thread          — UI, message loop, window rendering
DataCollector thread — GetIfTable2 polling at configured interval
Persistence thread   — Auto-save statistics every 5 minutes
```

Inter-thread communication:
- `DataCollector` → `Statistics` (direct call, mutex-protected)
- `DataCollector` → `TrafficMonitor` via `PostMessageW` (thread-safe UI update)
- `Persistence` uses `condition_variable` for sleep/wake

---

## Adding a Custom Icon

1. Create `resources/app.ico` (32×32, 16×16, 48×48 sizes)
2. Uncomment in `resources/resources.rc`:
   ```rc
   IDI_APP_ICON  ICON  "app.ico"
   ```
3. The tray icon will automatically load it via `LoadIconW(hInst, MAKEINTRESOURCEW(1))`

---

## Known Limitations & TODO

- [ ] GDI+ anti-aliased graph curves (optional improvement)
- [ ] Dark mode title bars (requires DwmSetWindowAttribute on Windows 10 1809+)
- [ ] Per-application traffic breakdown (requires ETW/WFP, more complex)
- [ ] IPv6 interface support (GetIfTable2 handles it, just filtering may need tuning)
