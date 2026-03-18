#include "statistics.h"
#include "../utils/format_helpers.h"
#include <algorithm>

namespace core {

void Statistics::update(uint64_t rx_delta, uint64_t tx_delta,
                        double rx_speed, double tx_speed) {
    uint32_t today = utils::today_key();

    std::lock_guard lock(mutex_);

    // Update today's record
    auto& day = days_[today];
    if (day.key == 0) {
        day.key = today;
        purge_old_records();
    }
    day.rx += rx_delta;
    day.tx += tx_delta;

    // Push speed sample into ring buffer
    speed_buf_[speed_head_] = { rx_speed, tx_speed };
    speed_head_ = (speed_head_ + 1) % kBufSize;
    if (speed_count_ < kBufSize) ++speed_count_;

    check_limits();
}

uint64_t Statistics::today_rx() const {
    uint32_t today = utils::today_key();
    std::lock_guard lock(mutex_);
    auto it = days_.find(today);
    return (it != days_.end()) ? it->second.rx : 0;
}

uint64_t Statistics::today_tx() const {
    uint32_t today = utils::today_key();
    std::lock_guard lock(mutex_);
    auto it = days_.find(today);
    return (it != days_.end()) ? it->second.tx : 0;
}

uint64_t Statistics::today_total() const {
    return today_rx() + today_tx();
}

uint64_t Statistics::month_rx() const {
    uint32_t month = utils::this_month_key();
    std::lock_guard lock(mutex_);
    uint64_t total = 0;
    for (const auto& [k, v] : days_) {
        if (k / 100 == month) total += v.rx;
    }
    return total;
}

uint64_t Statistics::month_tx() const {
    uint32_t month = utils::this_month_key();
    std::lock_guard lock(mutex_);
    uint64_t total = 0;
    for (const auto& [k, v] : days_) {
        if (k / 100 == month) total += v.tx;
    }
    return total;
}

uint64_t Statistics::month_total() const {
    return month_rx() + month_tx();
}

uint64_t Statistics::alltime_rx() const {
    std::lock_guard lock(mutex_);
    uint64_t total = 0;
    for (const auto& [k, v] : days_) total += v.rx;
    return total;
}

uint64_t Statistics::alltime_tx() const {
    std::lock_guard lock(mutex_);
    uint64_t total = 0;
    for (const auto& [k, v] : days_) total += v.tx;
    return total;
}

uint64_t Statistics::alltime_total() const {
    return alltime_rx() + alltime_tx();
}

std::vector<SpeedSample> Statistics::get_speed_history(int count) const {
    std::lock_guard lock(mutex_);
    int n = std::min(count, speed_count_);
    std::vector<SpeedSample> result(n);

    // Read from oldest to newest
    for (int i = 0; i < n; ++i) {
        int idx = (speed_head_ - n + i + kBufSize) % kBufSize;
        result[i] = speed_buf_[idx];
    }
    return result;
}

std::vector<DayRecord> Statistics::get_day_records(int max_days) const {
    std::lock_guard lock(mutex_);
    std::vector<DayRecord> result;
    result.reserve(days_.size());
    for (const auto& [k, v] : days_) {
        result.push_back(v);
    }
    // Sort descending by key (newest first)
    std::sort(result.begin(), result.end(),
        [](const DayRecord& a, const DayRecord& b) { return a.key > b.key; });

    if (static_cast<int>(result.size()) > max_days) {
        result.resize(max_days);
    }
    return result;
}

void Statistics::load_days(std::map<uint32_t, DayRecord> days) {
    std::lock_guard lock(mutex_);
    days_ = std::move(days);
}

void Statistics::check_limits() {
    // Called with mutex held
    if (daily_limit_ > 0) {
        uint32_t today = utils::today_key();
        auto it = days_.find(today);
        uint64_t used = (it != days_.end()) ? it->second.total() : 0;

        LimitAlert alert = LimitAlert::None;
        if (used >= daily_limit_)
            alert = LimitAlert::Exceeded;
        else if (used >= daily_limit_ * 8 / 10) // 80%
            alert = LimitAlert::Approaching;

        if (alert != last_daily_alert_) {
            last_daily_alert_ = alert;
            if (alert_cb_ && alert != LimitAlert::None) {
                // We call outside lock to avoid deadlock, but for simplicity we do it here
                // In production, post to main thread
                alert_cb_(alert, last_monthly_alert_);
            }
        }
    }

    if (monthly_limit_ > 0) {
        uint32_t month = utils::this_month_key();
        uint64_t used = 0;
        for (const auto& [k, v] : days_) {
            if (k / 100 == month) used += v.total();
        }

        LimitAlert alert = LimitAlert::None;
        if (used >= monthly_limit_)
            alert = LimitAlert::Exceeded;
        else if (used >= monthly_limit_ * 8 / 10)
            alert = LimitAlert::Approaching;

        if (alert != last_monthly_alert_) {
            last_monthly_alert_ = alert;
            if (alert_cb_ && alert != LimitAlert::None) {
                alert_cb_(last_daily_alert_, alert);
            }
        }
    }
}

void Statistics::purge_old_records() {
    // Keep only last 31 days; called with mutex held
    constexpr int kMaxDays = 31;
    if (static_cast<int>(days_.size()) <= kMaxDays) return;

    while (static_cast<int>(days_.size()) > kMaxDays) {
        days_.erase(days_.begin()); // Erase oldest (smallest key)
    }
}

} // namespace core
