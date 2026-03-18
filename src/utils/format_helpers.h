#pragma once
#include <windows.h>
#include <string>
#include <cstdio>
#include <cstdint>

namespace utils {

//-----------------------------------------------------------------------------
// Format byte count as human-readable string
// e.g. 1536 -> "1.50 KB", 2097152 -> "2.00 MB"
//-----------------------------------------------------------------------------
inline std::wstring format_bytes(uint64_t bytes) {
    wchar_t buf[64];
    if (bytes < 1024ULL)
        swprintf(buf, 64, L"%llu B", static_cast<unsigned long long>(bytes));
    else if (bytes < 1024ULL * 1024)
        swprintf(buf, 64, L"%.2f KB", bytes / 1024.0);
    else if (bytes < 1024ULL * 1024 * 1024)
        swprintf(buf, 64, L"%.2f MB", bytes / (1024.0 * 1024));
    else
        swprintf(buf, 64, L"%.2f GB", bytes / (1024.0 * 1024 * 1024));
    return buf;
}

//-----------------------------------------------------------------------------
// Format speed (bytes/sec) as compact overlay string
// e.g.  512 -> "512 B/s", 1536 -> "1.5K/s", 2097152 -> "2.0M/s"
//-----------------------------------------------------------------------------
inline std::wstring format_speed(double bps) {
    wchar_t buf[64];
    if (bps < 1024.0)
        swprintf(buf, 64, L"%.0fB/s", bps);
    else if (bps < 1024.0 * 1024)
        swprintf(buf, 64, L"%.1fK/s", bps / 1024.0);
    else if (bps < 1024.0 * 1024 * 1024)
        swprintf(buf, 64, L"%.1fM/s", bps / (1024.0 * 1024));
    else
        swprintf(buf, 64, L"%.2fG/s", bps / (1024.0 * 1024 * 1024));
    return buf;
}

//-----------------------------------------------------------------------------
// Format a date as YYYY-MM-DD
//-----------------------------------------------------------------------------
inline std::wstring format_date(const SYSTEMTIME& st) {
    wchar_t buf[32];
    swprintf(buf, 32, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return buf;
}

//-----------------------------------------------------------------------------
// Format datetime as YYYY-MM-DD HH:MM
//-----------------------------------------------------------------------------
inline std::wstring format_datetime(const SYSTEMTIME& st) {
    wchar_t buf[32];
    swprintf(buf, 32, L"%04d-%02d-%02d %02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

//-----------------------------------------------------------------------------
// Format limit percentage
//-----------------------------------------------------------------------------
inline std::wstring format_percent(double value, double total) {
    if (total <= 0.0) return L"0%";
    wchar_t buf[32];
    swprintf(buf, 32, L"%.1f%%", value / total * 100.0);
    return buf;
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
