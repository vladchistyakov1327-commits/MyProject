#pragma once
#include "statistics.h"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <map>

namespace core {

//-----------------------------------------------------------------------------
// App settings (persisted to settings.ini)
//-----------------------------------------------------------------------------
enum class OverlayPosition {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

enum class TextColor { Red, Green, Blue, White, Yellow };
enum class UpdateInterval { Half = 500, One = 1000, Two = 2000, Five = 5000 };
enum class InterfaceType { All, WiFi, Ethernet, Mobile, Specific };

struct AppSettings {
    OverlayPosition overlay_pos{ OverlayPosition::TopRight };
    TextColor       text_color{ TextColor::White };
    UpdateInterval  update_interval{ UpdateInterval::One };
    InterfaceType   iface_type{ InterfaceType::All };
    DWORD           specific_iface_index{ 0 };
    std::wstring    specific_iface_name;
    uint64_t        daily_limit_mb{ 0 };    // 0 = no limit
    uint64_t        monthly_limit_mb{ 0 };  // 0 = no limit
    bool            start_minimized{ false };
    bool            start_with_windows{ false };
    bool            overlay_visible{ true };
    int             overlay_offset_x{ 10 };
    int             overlay_offset_y{ 10 };
    int             font_size{ 14 };
};

//-----------------------------------------------------------------------------
// Persistence — loads/saves stats and settings, runs background save thread
//-----------------------------------------------------------------------------
class Persistence {
public:
    Persistence() = default;
    ~Persistence();

    // Set data directory (defaults to %APPDATA%\TrafficMonitor)
    void set_data_dir(const std::wstring& dir);
    const std::wstring& data_dir() const { return data_dir_; }

    // Initialize (create directories, etc.)
    bool initialize();

    // Settings
    bool load_settings(AppSettings& out);
    bool save_settings(const AppSettings& settings);

    // Statistics
    bool load_stats(std::map<uint32_t, DayRecord>& out);
    bool save_stats(const std::map<uint32_t, DayRecord>& days);

    // Background auto-save (calls save every N minutes)
    void start_autosave(Statistics& stats, int interval_minutes = 5);
    void stop_autosave();

    // Export data
    bool export_csv(const std::vector<DayRecord>& records, const std::wstring& path);
    bool export_txt(const std::vector<DayRecord>& records, const std::wstring& path);

private:
    std::wstring stats_path()    const;
    std::wstring settings_path() const;

    std::wstring data_dir_;

    // Auto-save
    std::thread             save_thread_;
    std::atomic<bool>       save_running_{ false };
    std::mutex              save_mutex_;
    std::condition_variable save_cv_;
};

} // namespace core
