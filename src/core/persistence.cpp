#include "persistence.h"
#include "../utils/logger.h"
#include "../utils/format_helpers.h"
#include <fstream>
#include <sstream>
#include <format>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

namespace core {

//=============================================================================
// Persistence destructor
//=============================================================================
Persistence::~Persistence() {
    stop_autosave();
}

void Persistence::set_data_dir(const std::wstring& dir) {
    data_dir_ = dir;
}

bool Persistence::initialize() {
    if (data_dir_.empty()) {
        wchar_t app_data[MAX_PATH] = {};
        if (!SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, app_data))) {
            LOG_ERROR("Failed to get APPDATA path");
            return false;
        }
        data_dir_ = std::wstring(app_data) + L"\\TrafficMonitor";
    }

    if (!CreateDirectoryW(data_dir_.c_str(), nullptr)) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            LOG_ERROR("Failed to create data directory");
            return false;
        }
    }

    LOG_INFO(std::format("Data directory: {}",
             std::string(data_dir_.begin(), data_dir_.end())));
    return true;
}

//=============================================================================
// Stats binary format:
// Header: "TRAF" magic (4 bytes) + version (4 bytes) + count (4 bytes)
// Per record: key (4 bytes) + rx (8 bytes) + tx (8 bytes)
//=============================================================================
static constexpr uint32_t kMagic   = 0x46415254; // "TRAF"
static constexpr uint32_t kVersion = 1;

bool Persistence::load_stats(std::map<uint32_t, DayRecord>& out) {
    std::ifstream f(stats_path(), std::ios::binary);
    if (!f.is_open()) {
        LOG_INFO("No stats file found, starting fresh");
        return true; // Not an error
    }

    uint32_t magic = 0, version = 0, count = 0;
    f.read(reinterpret_cast<char*>(&magic),   sizeof(magic));
    f.read(reinterpret_cast<char*>(&version), sizeof(version));
    f.read(reinterpret_cast<char*>(&count),   sizeof(count));

    if (magic != kMagic || version != kVersion) {
        LOG_WARN("Stats file format mismatch, ignoring");
        return false;
    }

    for (uint32_t i = 0; i < count; ++i) {
        DayRecord rec;
        f.read(reinterpret_cast<char*>(&rec.key), sizeof(rec.key));
        f.read(reinterpret_cast<char*>(&rec.rx),  sizeof(rec.rx));
        f.read(reinterpret_cast<char*>(&rec.tx),  sizeof(rec.tx));
        out[rec.key] = rec;
    }

    LOG_INFO(std::format("Loaded {} day records", count));
    return true;
}

bool Persistence::save_stats(const std::map<uint32_t, DayRecord>& days) {
    // Write to temp file first, then rename (atomic on same volume)
    std::wstring tmp_path = stats_path() + L".tmp";
    std::ofstream f(tmp_path, std::ios::binary | std::ios::trunc);
    if (!f.is_open()) {
        LOG_ERROR("Failed to open stats temp file for writing");
        return false;
    }

    uint32_t count = static_cast<uint32_t>(days.size());
    f.write(reinterpret_cast<const char*>(&kMagic),   sizeof(kMagic));
    f.write(reinterpret_cast<const char*>(&kVersion), sizeof(kVersion));
    f.write(reinterpret_cast<const char*>(&count),    sizeof(count));

    for (const auto& [k, v] : days) {
        f.write(reinterpret_cast<const char*>(&v.key), sizeof(v.key));
        f.write(reinterpret_cast<const char*>(&v.rx),  sizeof(v.rx));
        f.write(reinterpret_cast<const char*>(&v.tx),  sizeof(v.tx));
    }
    f.close();

    // Atomic rename
    std::wstring final_path = stats_path();
    DeleteFileW(final_path.c_str());
    MoveFileW(tmp_path.c_str(), final_path.c_str());

    return true;
}

//=============================================================================
// Settings INI format (simple key=value)
//=============================================================================
bool Persistence::load_settings(AppSettings& out) {
    std::wstring path = settings_path();
    std::wifstream f(path);
    if (!f.is_open()) return true; // Use defaults

    std::wstring line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == L';' || line[0] == L'[') continue;
        auto eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;
        std::wstring key = line.substr(0, eq);
        std::wstring val = line.substr(eq + 1);

        if (key == L"overlay_pos")        out.overlay_pos       = static_cast<OverlayPosition>(std::stoi(val));
        else if (key == L"text_color")    out.text_color        = static_cast<TextColor>(std::stoi(val));
        else if (key == L"update_ms")     out.update_interval   = static_cast<UpdateInterval>(std::stoi(val));
        else if (key == L"iface_type")    out.iface_type        = static_cast<InterfaceType>(std::stoi(val));
        else if (key == L"iface_index")   out.specific_iface_index = static_cast<DWORD>(std::stoul(val));
        else if (key == L"iface_name")    out.specific_iface_name  = val;
        else if (key == L"daily_mb")      out.daily_limit_mb    = std::stoull(val);
        else if (key == L"monthly_mb")    out.monthly_limit_mb  = std::stoull(val);
        else if (key == L"start_min")     out.start_minimized   = (val == L"1");
        else if (key == L"start_win")     out.start_with_windows= (val == L"1");
        else if (key == L"overlay_vis")   out.overlay_visible   = (val == L"1");
        else if (key == L"offset_x")      out.overlay_offset_x  = std::stoi(val);
        else if (key == L"offset_y")      out.overlay_offset_y  = std::stoi(val);
        else if (key == L"font_size")     out.font_size         = std::stoi(val);
    }
    return true;
}

bool Persistence::save_settings(const AppSettings& s) {
    std::wofstream f(settings_path(), std::ios::trunc);
    if (!f.is_open()) {
        LOG_ERROR("Failed to open settings file for writing");
        return false;
    }
    f << L"[TrafficMonitor]\n";
    f << L"overlay_pos="   << static_cast<int>(s.overlay_pos)       << L"\n";
    f << L"text_color="    << static_cast<int>(s.text_color)         << L"\n";
    f << L"update_ms="     << static_cast<int>(s.update_interval)    << L"\n";
    f << L"iface_type="    << static_cast<int>(s.iface_type)         << L"\n";
    f << L"iface_index="   << s.specific_iface_index                 << L"\n";
    f << L"iface_name="    << s.specific_iface_name                  << L"\n";
    f << L"daily_mb="      << s.daily_limit_mb                       << L"\n";
    f << L"monthly_mb="    << s.monthly_limit_mb                     << L"\n";
    f << L"start_min="     << (s.start_minimized    ? L"1" : L"0")   << L"\n";
    f << L"start_win="     << (s.start_with_windows ? L"1" : L"0")   << L"\n";
    f << L"overlay_vis="   << (s.overlay_visible    ? L"1" : L"0")   << L"\n";
    f << L"offset_x="      << s.overlay_offset_x                     << L"\n";
    f << L"offset_y="      << s.overlay_offset_y                     << L"\n";
    f << L"font_size="     << s.font_size                             << L"\n";
    return true;
}

//=============================================================================
// Auto-save
//=============================================================================
void Persistence::start_autosave(Statistics& stats, int interval_minutes) {
    if (save_running_.load()) return;
    save_running_ = true;
    save_thread_ = std::thread([this, &stats, interval_minutes]() {
        while (save_running_.load()) {
            std::unique_lock lock(save_mutex_);
            save_cv_.wait_for(lock,
                std::chrono::minutes(interval_minutes),
                [this] { return !save_running_.load(); });
            if (!save_running_.load()) break;
            save_stats(stats.raw_days());
            LOG_INFO("Auto-saved statistics");
        }
    });
}

void Persistence::stop_autosave() {
    if (!save_running_.load()) return;
    save_running_ = false;
    save_cv_.notify_all();
    if (save_thread_.joinable()) save_thread_.join();
}

//=============================================================================
// Export
//=============================================================================
bool Persistence::export_csv(const std::vector<DayRecord>& records,
                              const std::wstring& path) {
    std::wofstream f(path, std::ios::trunc);
    if (!f.is_open()) return false;

    f << L"Date,Downloaded,Uploaded,Total\n";
    for (const auto& r : records) {
        uint32_t y = r.key / 10000;
        uint32_t m = (r.key / 100) % 100;
        uint32_t d = r.key % 100;
        f << std::format(L"{:04d}-{:02d}-{:02d},{},{},{}\n",
            y, m, d, r.rx, r.tx, r.total());
    }
    return true;
}

bool Persistence::export_txt(const std::vector<DayRecord>& records,
                              const std::wstring& path) {
    std::wofstream f(path, std::ios::trunc);
    if (!f.is_open()) return false;

    f << L"Traffic Monitor - Export\n";
    f << L"========================\n\n";
    f << std::format(L"{:<12} {:<14} {:<14} {:<14}\n",
                     L"Date", L"Downloaded", L"Uploaded", L"Total");
    f << std::wstring(55, L'-') << L"\n";

    for (const auto& r : records) {
        uint32_t y = r.key / 10000;
        uint32_t m = (r.key / 100) % 100;
        uint32_t d = r.key % 100;
        f << std::format(L"{:04d}-{:02d}-{:02d}   {:<14} {:<14} {:<14}\n",
            y, m, d,
            utils::format_bytes(r.rx),
            utils::format_bytes(r.tx),
            utils::format_bytes(r.total()));
    }
    return true;
}

//=============================================================================
// Private helpers
//=============================================================================
std::wstring Persistence::stats_path() const {
    return data_dir_ + L"\\stats.dat";
}

std::wstring Persistence::settings_path() const {
    return data_dir_ + L"\\settings.ini";
}

} // namespace core
