#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <windows.h>
#include <iphlpapi.h>

namespace core {

//-----------------------------------------------------------------------------
// Represents a single network interface
//-----------------------------------------------------------------------------
struct NetworkInterface {
    DWORD       index{ 0 };
    std::wstring name;          // Friendly name (e.g. "Wi-Fi")
    std::wstring description;   // Adapter description
    IF_TYPE     type{ IF_TYPE_OTHER };
    bool        is_up{ false };
};

//-----------------------------------------------------------------------------
// Current traffic snapshot for one interface
//-----------------------------------------------------------------------------
struct TrafficSnapshot {
    uint64_t rx_bytes{ 0 };   // Total received bytes
    uint64_t tx_bytes{ 0 };   // Total transmitted bytes
    double   rx_speed{ 0.0 }; // Bytes per second (down)
    double   tx_speed{ 0.0 }; // Bytes per second (up)
    uint64_t rx_delta{ 0 };   // Bytes received since last snapshot
    uint64_t tx_delta{ 0 };   // Bytes sent since last snapshot
    double   interval_sec{ 0.0 }; // Actual interval since last measurement
};

//-----------------------------------------------------------------------------
// Aggregated interface type selection
//-----------------------------------------------------------------------------
enum class InterfaceFilter {
    All,
    WiFi,
    Ethernet,
    Mobile,
    Specific  // Use specific interface index
};

//-----------------------------------------------------------------------------
// DataCollector — collects network traffic data in a background thread
//-----------------------------------------------------------------------------
class DataCollector {
public:
    using Callback = std::function<void(const TrafficSnapshot&)>;

    DataCollector() = default;
    ~DataCollector();

    // Enumerate available interfaces
    static std::vector<NetworkInterface> enumerate_interfaces();

    // Configure before start()
    void set_filter(InterfaceFilter filter) { filter_ = filter; }
    void set_interface_index(DWORD index)   { specific_index_ = index; }
    void set_interval_ms(int ms)            { interval_ms_ = ms; }
    void set_callback(Callback cb)          { callback_ = std::move(cb); }

    // Start/stop background collection
    bool start();
    void stop();

    bool is_running() const { return running_.load(); }

    // Access current snapshot (thread-safe)
    TrafficSnapshot current_snapshot() const {
        std::lock_guard lock(snapshot_mutex_);
        return snapshot_;
    }

private:
    void thread_proc();
    uint64_t collect_bytes(bool rx) const;

    InterfaceFilter filter_{ InterfaceFilter::All };
    DWORD           specific_index_{ 0 };
    int             interval_ms_{ 1000 };
    Callback        callback_;

    mutable std::mutex    snapshot_mutex_;
    TrafficSnapshot       snapshot_;

    std::thread           thread_;
    std::atomic<bool>     running_{ false };
    HANDLE                stop_event_{ nullptr };

    // Previous raw counters for delta calculation
    uint64_t prev_rx_{ 0 };
    uint64_t prev_tx_{ 0 };
    LARGE_INTEGER prev_time_{};
    LARGE_INTEGER freq_{};
};

} // namespace core
