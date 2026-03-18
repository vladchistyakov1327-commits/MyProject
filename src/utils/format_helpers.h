#pragma once
#include <windows.h>
#include <string>
#include <format>
#include <cstdint>

namespace utils {

//-----------------------------------------------------------------------------
// Format byte count as human-readable string
// e.g. 1536 -> "1.50 KB", 2097152 -> "2.00 MB"
//-----------------------------------------------------------------------------
inline std::wstring format_bytes(uint64_t bytes) {
    if (bytes < 1024ULL)
        return std::format(L"{} B", bytes);
    if (bytes < 1024ULL * 1024)
        return std::format(L"{:.2f} KB", bytes / 1024.0);
    if (bytes < 1024ULL * 1024 * 1024)
        return std::format(L"{:.2f} MB", bytes / (1024.0 * 1024));
    return std::format(L"{:.2f} GB", bytes / (1024.0 * 1024 * 1024));
}

//-----------------------------------------------------------------------------
// Format speed (bytes/sec) as compact overlay string
// e.g.  512 -> "512 B/s", 1536 -> "1.5K/s", 2097152 -> "2.0M/s"
//-----------------------------------------------------------------------------
inline std::wstring format_speed(double bps) {
    if (bps < 1024.0)
        return std::format(L"{:.0f}B/s", bps);
    if (bps < 1024.0 * 1024)
        return std::format(L"{:.1f}K/s", bps / 1024.0);
    if (bps < 1024.0 * 1024 * 1024)
        return std::format(L"{:.1f}M/s", bps / (1024.0 * 1024));
    return std::format(L"{:.2f}G/s", bps / (1024.0 * 1024 * 1024));
}

//-----------------------------------------------------------------------------
// Format a date as YYYY-MM-DD
//-----------------------------------------------------------------------------
inline std::wstring format_date(const SYSTEMTIME& st) {
    return std::format(L"{:04d}-{:02d}-{:02d}", st.wYear, st.wMonth, st.wDay);
}

//-----------------------------------------------------------------------------
// Format datetime as YYYY-MM-DD HH:MM
//-----------------------------------------------------------------------------
inline std::wstring format_datetime(const SYSTEMTIME& st) {
    return std::format(L"{:04d}-{:02d}-{:02d} {:02d}:{:02d}",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
}

//-----------------------------------------------------------------------------
// Format limit percentage
//-----------------------------------------------------------------------------
inline std::wstring format_percent(double value, double total) {
    if (total <= 0.0) return L"0%";
    return std::format(L"{:.1f}%", value / total * 100.0);
}

//-----------------------------------------------------------------------------
// Current local date key as YYYYMMDD integer (for map keys)
//-----------------------------------------------------------------------------
inline uint32_t today_key() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    return static_cast<uint32_t>(st.wYear) * 10000
         + static_cast<uint32_t>(st.wMonth) * 100
         + static_cast<uint32_t>(st.wDay);
}

//-----------------------------------------------------------------------------
// Current local month key as YYYYMM integer
//-----------------------------------------------------------------------------
inline uint32_t this_month_key() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    return static_cast<uint32_t>(st.wYear) * 100
         + static_cast<uint32_t>(st.wMonth);
}

} // namespace utils
