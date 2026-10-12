#pragma once
#include <ArduinoJson.h>
#include "../comms/lora.h"
#include <cmath>
#include <cstring>
#include <cstdio>

namespace kaelix::receiver {
struct Event {
    comms::LoraPacket packet{};
    uint32_t sequence = 0;
    uint64_t received_ms = 0;
    char received_at[25]{};
    float rssi = 0, snr = 0;
};
template<size_t Capacity> class EventQueue {
    Event events[Capacity]{};
    size_t head = 0, used = 0;
public:
    size_t size() const { return used; }
    bool push(const Event& e) {
        if (used == Capacity) return false;
        events[(head + used++) % Capacity] = e;
        return true;
    }
    size_t snapshot(Event* out, size_t maximum) const {
        size_t n = used < maximum ? used : maximum;
        for (size_t i = 0; i < n; ++i) out[i] = events[(head+i)%Capacity];
        return n;
    }
    void pop(size_t n) { if (n <= used) { head = (head+n)%Capacity; used -= n; } }
};
inline void add_reading(JsonObject o, const Event& e) {
    o["event_seq"] = e.sequence;
    o["gateway_rx_uptime_ms"] = e.received_ms;
    if (e.received_at[0]) o["gateway_received_at"] = e.received_at;
    else o["gateway_received_at"] = nullptr;
    o["packet_version"] = e.packet.version;
    char id[9]; snprintf(id, sizeof(id), "%08lX", (unsigned long)e.packet.device_id);
    o["device_id"] = id;
    o["device_sequence"] = e.packet.boot_count;
    o["state"] = machine_state_to_string(static_cast<MachineState>(e.packet.state));
    o["diag_code"] = e.packet.diag;
    if (std::isfinite(e.packet.rms)) o["rms_g"] = static_cast<double>(e.packet.rms);
    else o["rms_g"] = nullptr;
    if (std::isfinite(e.packet.temperature_c)) o["temperature_c"] = static_cast<double>(e.packet.temperature_c);
    else o["temperature_c"] = nullptr;
    o["rssi_dbm"] = e.rssi;
    o["snr_db"] = e.snr;
    o["packet_crc16"] = e.packet.crc;
}
inline bool valid_utc(const char* at) {
    const size_t n=strlen(at);
    if(n<20 || n>64 || at[4]!='-' || at[7]!='-' || at[10]!='T' || at[13]!=':' || at[16]!=':' || at[n-1]!='Z') return false;
    const int positions[]={0,1,2,3,5,6,8,9,11,12,14,15,17,18};
    for(int i:positions) if(at[i]<'0' || at[i]>'9') return false;
    if(n>20) {
        if(at[19]!='.' || n<22) return false;
        for(size_t i=20;i<n-1;++i) if(at[i]<'0' || at[i]>'9') return false;
    }
    auto pair=[&](size_t i) { return (at[i]-'0')*10+at[i+1]-'0'; };
    const int year=pair(0)*100+pair(2), month=pair(5), day=pair(8);
    if(month<1 || month>12 || pair(11)>23 || pair(14)>59 || pair(17)>59) return false;
    const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    const bool leap=year%4==0 && (year%100!=0 || year%400==0);
    return day>=1 && day<=days[month-1]+(month==2 && leap ? 1 : 0);
}
inline bool response_identity(JsonVariantConst d, const char* gateway, const char* session) {
    const char* at = d["server_received_at"] | "";
    return d["schema_version"].is<int>() && d["schema_version"].as<int>() == 1 &&
        strcmp(d["gateway_id"] | "", gateway) == 0 &&
        strcmp(d["gateway_session_id"] | "", session) == 0 &&
        valid_utc(at);
}
inline bool acknowledgements_valid(JsonVariantConst d, const char* gateway,
                                   const char* session, const Event* batch, size_t count) {
    if (!response_identity(d, gateway, session) || d.size()!=5) return false;
    auto a = d["acknowledged"].as<JsonArrayConst>();
    if (a.size() != count || !count || count > 8) return false;
    bool seen[8]{};
    for (JsonObjectConst item : a) {
        if (item.size()!=2 || !item["event_seq"].is<uint32_t>()) return false;
        const char* status = item["status"] | "";
        if (strcmp(status, "stored") && strcmp(status, "duplicate")) return false;
        bool matched = false;
        for (size_t i=0; i<count; ++i) if (batch[i].sequence == item["event_seq"].as<uint32_t>()) {
            if (seen[i]) return false;
            seen[i] = true; matched = true; break;
        }
        if (!matched) return false;
    }
    return true;
}
}
