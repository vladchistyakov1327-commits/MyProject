#include "data_collector.h"
#include "../utils/logger.h"
#include <algorithm>
#include <chrono>

#pragma comment(lib, "iphlpapi.lib")

namespace core {

DataCollector::~DataCollector() {
    stop();
}

std::vector<NetworkInterface> DataCollector::enumerate_interfaces() {
    std::vector<NetworkInterface> result;

    ULONG flags  = GAA_FLAG_INCLUDE_ALL_INTERFACES;
    ULONG family = AF_UNSPEC;
    ULONG size   = 0;

    // First call to get required buffer size
    GetAdaptersAddresses(family, flags, nullptr, nullptr, &size);

    std::vector<uint8_t> buffer(size);
    auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    if (GetAdaptersAddresses(family, flags, nullptr, adapter, &size) != NO_ERROR) {
        LOG_ERROR("GetAdaptersAddresses failed");
        return result;
    }

    for (auto* a = adapter; a != nullptr; a = a->Next) {
        NetworkInterface iface;
        iface.index       = a->IfIndex;
        iface.type        = a->IfType;
        iface.is_up       = (a->OperStatus == IfOperStatusUp);
        iface.description = a->Description ? a->Description : L"";

        // Friendly name
        if (a->FriendlyName) {
            iface.name = a->FriendlyName;
        }

        result.push_back(iface);
    }

    return result;
}

bool DataCollector::start() {
    if (running_.load()) return true;

    stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stop_event_) {
        LOG_ERROR("Failed to create stop event");
        return false;
    }

    QueryPerformanceFrequency(&freq_);
    QueryPerformanceCounter(&prev_time_);
    prev_rx_ = collect_bytes(true);
    prev_tx_ = collect_bytes(false);

    running_ = true;
    thread_ = std::thread(&DataCollector::thread_proc, this);
    LOG_INFO("DataCollector started");
    return true;
}

void DataCollector::stop() {
    if (!running_.load()) return;
    running_ = false;
    if (stop_event_) {
        SetEvent(stop_event_);
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    if (stop_event_) {
        CloseHandle(stop_event_);
        stop_event_ = nullptr;
    }
    LOG_INFO("DataCollector stopped");
}

void DataCollector::thread_proc() {
    while (running_.load()) {
        DWORD wait_res = WaitForSingleObject(stop_event_, interval_ms_);
        if (wait_res == WAIT_OBJECT_0) break; // Stop requested

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        uint64_t cur_rx = collect_bytes(true);
        uint64_t cur_tx = collect_bytes(false);

        double elapsed = static_cast<double>(now.QuadPart - prev_time_.QuadPart)
                       / static_cast<double>(freq_.QuadPart);

        if (elapsed < 0.001) elapsed = 0.001; // Guard against zero

        uint64_t rx_delta = (cur_rx >= prev_rx_) ? (cur_rx - prev_rx_) : 0;
        uint64_t tx_delta = (cur_tx >= prev_tx_) ? (cur_tx - prev_tx_) : 0;

        TrafficSnapshot snap;
        snap.rx_bytes      = cur_rx;
        snap.tx_bytes      = cur_tx;
        snap.rx_delta      = rx_delta;
        snap.tx_delta      = tx_delta;
        snap.rx_speed      = static_cast<double>(rx_delta) / elapsed;
        snap.tx_speed      = static_cast<double>(tx_delta) / elapsed;
        snap.interval_sec  = elapsed;

        {
            std::lock_guard lock(snapshot_mutex_);
            snapshot_ = snap;
        }

        prev_rx_   = cur_rx;
        prev_tx_   = cur_tx;
        prev_time_ = now;

        if (callback_) {
            callback_(snap);
        }
    }
}

uint64_t DataCollector::collect_bytes(bool rx) const {
    // Use MIB_IF_TABLE2 for 64-bit counters
    MIB_IF_TABLE2* table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR) {
        LOG_ERROR("GetIfTable2 failed");
        return 0;
    }

    uint64_t total = 0;

    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const auto& row = table->Table[i];

        bool include = false;

        switch (filter_) {
            case InterfaceFilter::All:
                // Include all up, non-loopback, non-tunnel interfaces
                include = (row.OperStatus == IfOperStatusUp)
                       && (row.Type != IF_TYPE_SOFTWARE_LOOPBACK)
                       && (row.Type != IF_TYPE_TUNNEL);
                break;

            case InterfaceFilter::WiFi:
                include = (row.OperStatus == IfOperStatusUp)
                       && (row.Type == IF_TYPE_IEEE80211);
                break;

            case InterfaceFilter::Ethernet:
                include = (row.OperStatus == IfOperStatusUp)
                       && (row.Type == IF_TYPE_ETHERNET_CSMACD);
                break;

            case InterfaceFilter::Mobile:
                // WWAN / mobile broadband
                include = (row.OperStatus == IfOperStatusUp)
                       && (row.Type == IF_TYPE_WWANPP || row.Type == IF_TYPE_WWANPP2);
                break;

            case InterfaceFilter::Specific:
                include = (row.InterfaceIndex == specific_index_);
                break;
        }

        if (include) {
            total += rx ? row.InOctets : row.OutOctets;
        }
    }

    FreeMibTable(table);
    return total;
}

} // namespace core
