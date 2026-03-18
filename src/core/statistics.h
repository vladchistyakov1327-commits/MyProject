#pragma once
#include <cstdint>
#include <map>
#include <array>
#include <vector>
#include <string>
#include <mutex>
#include <functional>
#include <windows.h>

namespace core {

//-----------------------------------------------------------------------------
// Daily statistics record
//-----------------------------------------------------------------------------
struct DayRecord {
    uint32_t key{ 0 };      // YYYYMMDD
    uint64_t rx{ 0 };       // Received bytes
    uint64_t tx{ 0 };       // Transmitted bytes

    uint64_t total() const { return rx + tx; }
};

//-----------------------------------------------------------------------------
// Speed history for graph rendering (circular buffer per minute)
//-----------------------------------------------------------------------------
struct SpeedSample {
    double rx{ 0.0 };
    double tx{ 0.0 };
};

constexpr int kHourSamples  = 3600; // one per second, one hour
constexpr int kDaySamples   = 1440; // one per minute, one day

//-----------------------------------------------------------------------------
// Alert type
//-----------------------------------------------------------------------------
enum class LimitAlert { None, Approaching, Exceeded };

//-----------------------------------------------------------------------------
// Statistics — thread-safe, tracks per-day and per-month totals
//-----------------------------------------------------------------------------
class Statistics {
public:
    using AlertCallback = std::function<void(LimitAlert daily, LimitAlert monthly)>;

    Statistics() = default;

    // Configuration
    void set_daily_limit(uint64_t bytes)   { daily_limit_   = bytes; }
    void set_monthly_limit(uint64_t bytes) { monthly_limit_ = bytes; }
    void set_alert_callback(AlertCallback cb) { alert_cb_ = std::move(cb); }

    // Called from DataCollector callback
    void update(uint64_t rx_delta, uint64_t tx_delta,
                double rx_speed, double tx_speed);

    // Query current day totals
    uint64_t today_rx() const;
    uint64_t today_tx() const;
    uint64_t today_total() const;

    // Query current month totals
    uint64_t month_rx() const;
    uint64_t month_tx() const;
    uint64_t month_total() const;

    // Query all-time totals
    uint64_t alltime_rx() const;
    uint64_t alltime_tx() const;
    uint64_t alltime_total() const;

    // Recent speed samples (copy for rendering)
    std::vector<SpeedSample> get_speed_history(int count) const;

    // Day records (last N days)
    std::vector<DayRecord> get_day_records(int max_days = 31) const;

    // Limits
    uint64_t daily_limit()   const { return daily_limit_; }
    uint64_t monthly_limit() const { return monthly_limit_; }

    // Access raw day map for persistence
    const std::map<uint32_t, DayRecord>& raw_days() const { return days_; }
    void load_days(std::map<uint32_t, DayRecord> days);

private:
    void check_limits();
    void purge_old_records();

    mutable std::mutex mutex_;

    // Day records: key = YYYYMMDD
    std::map<uint32_t, DayRecord> days_;

    // Speed ring buffer
    static constexpr int kBufSize = kHourSamples;
    std::array<SpeedSample, kBufSize> speed_buf_{};
    int    speed_head_{ 0 };
    int    speed_count_{ 0 };

    uint64_t daily_limit_{ 0 };   // 0 = no limit
    uint64_t monthly_limit_{ 0 };

    AlertCallback alert_cb_;

    // Prevent duplicate alerts
    LimitAlert last_daily_alert_{ LimitAlert::None };
    LimitAlert last_monthly_alert_{ LimitAlert::None };
};

} // namespace core
