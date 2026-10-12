#include "internet.h"
#include "delivery.h"
#include "log_buffer.h"
#include "../serial_log.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_sntp.h>
#include <esp_system.h>
#include <ctime>
#include <algorithm>
#include <atomic>
#if defined(KAELIX_GATEWAY_CONFIG_HEADER)
#include KAELIX_GATEWAY_CONFIG_HEADER
#elif __has_include("gateway_config.local.h")
#include "gateway_config.local.h"
#else
#include "gateway_config.example.h"
#endif

namespace kaelix::receiver::internet {
namespace {
constexpr size_t capacity = 64;
EventQueue<capacity> queue;
LogBuffer<64> diagnostic_logs;
portMUX_TYPE logs_mux=portMUX_INITIALIZER_UNLOCKED;
void capture_log(uint64_t ms,const char* level,const char* event,const char* message,bool truncated) {
    if (!strcmp(event,"LOGS")) return; // no diagnostic delivery feedback loop
    portENTER_CRITICAL(&logs_mux);
    diagnostic_logs.push(ms,level,event,message,truncated);
    portEXIT_CRITICAL(&logs_mux);
}
// Quarentena RAM de 8 eventos: preserva o lote permanente mais recente.
Event quarantine[8];
size_t quarantine_size = 0;
SemaphoreHandle_t mutex = nullptr;
char session[37];
uint32_t next_event = 0, heartbeat_sequence = 0;
uint64_t valid = 0, invalid = 0, spi = 0, dropped = 0, acknowledged = 0, http_failures = 0;
bool radio_initialized = false;
std::atomic<bool> clock_ready{false};
bool enabled = false, blocked = false;
uint64_t now_ms() { return esp_timer_get_time() / 1000ULL; }
void timestamp(char* out, size_t size) {
    out[0] = 0;
    if (!clock_ready.load()) return;
    time_t now = time(nullptr);
    tm utc{};
    gmtime_r(&now, &utc);
    strftime(out, size, "%Y-%m-%dT%H:%M:%SZ", &utc);
}
void sync_time(timeval*) { clock_ready.store(true); }
void lock() { xSemaphoreTake(mutex, portMAX_DELAY); }
void unlock() { xSemaphoreGive(mutex); }
void identity(JsonDocument& d) {
    d["schema_version"] = 1;
    d["gateway_id"] = KAELIX_GATEWAY_ID;
    d["gateway_session_id"] = session;
}
bool uuid(const char* s) {
    if (strlen(s) != 36) return false;
    for (size_t i=0; i<36; ++i) {
        if (i==8 || i==13 || i==18 || i==23) { if (s[i]!='-') return false; }
        else if (!((s[i]>='0' && s[i]<='9') || (s[i]>='a' && s[i]<='f'))) return false;
    }
    return true;
}
void create_session() {
    uint8_t b[16]; esp_fill_random(b, sizeof(b));
    b[6] = (b[6]&15) | 0x40; b[8] = (b[8]&63) | 0x80;
    snprintf(session, sizeof(session),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
}
// Leitura limitada para não alocar um corpo arbitrário enviado por servidor/proxy.
class BoundedStream : public Stream {
    Stream& source;
    size_t remaining;
    const size_t limit;
public:
    BoundedStream(Stream& s, size_t limit) : source(s), remaining(limit), limit(limit) { setTimeout(5000); }
    size_t consumed() const { return limit-remaining; }
    int available() override { return remaining ? source.available() : 0; }
    int read() override { if (!remaining) return -1; int c=source.read(); if(c>=0) --remaining; return c; }
    int peek() override { return remaining ? source.peek() : -1; }
    void flush() override {}
    size_t write(uint8_t) override { return 0; }
};
struct Result { int code = -1; uint64_t retry_ms = 0; bool json = false; };
Result post(const char* path, JsonDocument& request, JsonDocument& response) {
    Result result;
    if (request.overflowed()) return result;
    String payload;
    const size_t expected = measureJson(request);
    if (expected>32768 || !payload.reserve(expected) || serializeJson(request, payload)!=expected) return result;
    if (payload.length() > 32768) return result;
    HTTPClient http;
    WiFiClient plain;
    WiFiClientSecure tls;
    String base(KAELIX_API_BASE_URL);
    bool secure = base.startsWith("https://");
    if (secure) {
        if (!clock_ready.load()) return result;
        tls.setCACert(KAELIX_TLS_CA_PEM);
        tls.setHandshakeTimeout(5);
    }
    WiFiClient& client = secure ? static_cast<WiFiClient&>(tls) : plain;
    http.setConnectTimeout(5000);
    http.setTimeout(5000);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.useHTTP10(true); // resposta sem chunked para o parser do stream
    if (!http.begin(client, base + path)) return result;
    const char* keys[] = {"Retry-After"}; http.collectHeaders(keys, 1);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", String("Bearer ") + KAELIX_GATEWAY_TOKEN);
    result.code = http.POST(payload);
    String retry = http.header("Retry-After");
    // Segundos e HTTP-date; nunca antecipar um Retry-After reconhecido.
    if (retry.length()) {
        bool digits = true;
        for (size_t i=0; i<retry.length(); ++i) if (retry[i]<'0' || retry[i]>'9') digits=false;
        if (digits) result.retry_ms = std::min<uint64_t>(strtoull(retry.c_str(), nullptr, 10), UINT64_MAX/4000ULL)*1000ULL;
        else if (clock_ready.load()) {
            tm date{};
            if (strptime(retry.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &date)) {
                const int64_t delta = static_cast<int64_t>(mktime(&date)) - time(nullptr);
                if (delta > 0) result.retry_ms = static_cast<uint64_t>(delta)*1000ULL;
            }
        }
    }
    if (result.code == 200 && http.getSize() <= 4096) {
        BoundedStream stream(http.getStream(), 4096);
        result.json = !deserializeJson(response, stream, DeserializationOption::NestingLimit(6));
        // Não confirmar um prefixo JSON seguido de outro objeto ou lixo.
        const uint64_t deadline=now_ms()+5000;
        while (result.json) {
            if (http.getSize()>=0 && stream.consumed()==static_cast<size_t>(http.getSize())) break;
            if (!http.connected() && !stream.available()) {
                if (http.getSize()>=0) result.json=false; // corpo truncado
                break;
            }
            if (stream.consumed()>=4096 || now_ms()>=deadline) { result.json=false; break; }
            const int c=stream.read();
            if (c<0) { vTaskDelay(pdMS_TO_TICKS(1)); continue; }
            if (c!=' ' && c!='\r' && c!='\n' && c!='\t') result.json=false;
        }
    }
    http.end();
    return result;
}
bool send_logs() {
    RemoteLog batch[8];
    portENTER_CRITICAL(&logs_mux);
    const size_t count=diagnostic_logs.snapshot(batch,8);
    portEXIT_CRITICAL(&logs_mux);
    if (!count) return true;
    JsonDocument request,response; identity(request);
    auto logs=request["logs"].to<JsonArray>();
    for(size_t i=0;i<count;++i) {
        auto o=logs.add<JsonObject>();
        o["log_seq"]=batch[i].sequence;o["uptime_ms"]=batch[i].uptime_ms;
        o["level"]=batch[i].level;o["event"]=batch[i].event;
        o["message"]=batch[i].message;o["truncated"]=batch[i].truncated;
    }
    Result r=post("/api/v1/ingest/logs",request,response);
    auto ack=response["acknowledged"].as<JsonArrayConst>();
    bool ok=r.code==200 && r.json && response.size()==5 &&
        response_identity(response.as<JsonVariantConst>(),KAELIX_GATEWAY_ID,session) && ack.size()==count;
    bool seen[8]{};
    for(JsonVariantConst id:ack) {
        bool matched=false;
        if(id.is<uint32_t>()) for(size_t i=0;i<count;++i)if(batch[i].sequence==id.as<uint32_t>() && !seen[i]){seen[i]=true;matched=true;break;}
        if(!matched)ok=false;
    }
    if(ok) {
        portENTER_CRITICAL(&logs_mux);diagnostic_logs.pop(count);portEXIT_CRITICAL(&logs_mux);
    } else {
        logging::log("gateway","WARN","LOGS","diagnostic upload status=%d; measurements unaffected",r.code);
        if(r.code==400 || r.code==409 || r.code==413 || r.code==422) {
            // Bad diagnostics cannot obstruct telemetry or poison all following logs.
            portENTER_CRITICAL(&logs_mux);diagnostic_logs.pop(count);portEXIT_CRITICAL(&logs_mux);
        }
    }
    return ok;
}
void heartbeat(JsonDocument& d) {
    identity(d);
    d["heartbeat_seq"] = ++heartbeat_sequence;
    d["uptime_ms"] = now_ms();
    char at[25]; timestamp(at, sizeof(at));
    if (at[0]) d["sent_at"] = at; else d["sent_at"] = nullptr;
    d["firmware_version"] = "gateway-1.0.0";
    auto h = d["health"].to<JsonObject>();
    auto c = d["counters"].to<JsonObject>();
    lock();
    h["radio_initialized"] = radio_initialized;
    h["queue_size"] = queue.size();
    c["lora_valid"] = valid; c["lora_rejected"] = invalid; c["spi_errors"] = spi;
    c["http_failed_attempts"] = http_failures; c["readings_acknowledged"] = acknowledged;
    c["queue_dropped"] = dropped;
    unlock();
    h["wifi_connected"] = true; h["ntp_synchronized"] = clock_ready.load();
    h["queue_capacity"] = capacity; h["free_heap_bytes"] = ESP.getFreeHeap();
    h["wifi_rssi_dbm"] = WiFi.RSSI();
}
void worker(void*) {
    WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    WiFi.begin(KAELIX_WIFI_SSID, KAELIX_WIFI_PASSWORD);
    sntp_set_time_sync_notification_cb(sync_time);
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    uint64_t next_upload = 0, next_heartbeat = 0, next_wifi = now_ms()+15000;
    uint32_t backoff = 1000;
    uint64_t next_logs=0;
    uint32_t logs_backoff=5000;
    bool connected = false, synchronized = false;
    uint64_t next_ntp_notice = 0;
    for (;;) {
        const bool online = WiFi.status() == WL_CONNECTED;
        if (online != connected) {
            connected = online;
            logging::log("gateway", online?"INFO":"WARN", "WIFI", "%s", online?"connected":"disconnected");
        }
        if (clock_ready.load() && !synchronized) {
            synchronized=true;
            logging::log("gateway", "INFO", "NTP", "UTC synchronized");
        }
        if (online && !clock_ready.load() && String(KAELIX_API_BASE_URL).startsWith("https://") && now_ms()>=next_ntp_notice) {
            next_ntp_notice=now_ms()+30000;
            logging::log("gateway", "WARN", "NTP", "waiting for UTC synchronization before HTTPS");
        }
        if (!online) {
            if (now_ms() >= next_wifi) { WiFi.reconnect(); next_wifi = now_ms()+15000; }
        } else if (!blocked && (clock_ready.load() || String(KAELIX_API_BASE_URL).startsWith("http://")) && now_ms() >= next_upload) {
            Event batch[8];
            lock(); const size_t count = queue.snapshot(batch, 8); unlock();
            const bool send_heartbeat = now_ms() >= next_heartbeat;
            if (send_heartbeat && heartbeat_sequence == UINT32_MAX) {
                blocked=true;
                logging::log("gateway", "ERROR", "NET", "heartbeat sequence exhausted; reboot required for new session");
                continue;
            }
            if (count || send_heartbeat) {
                JsonDocument request, response;
                uint32_t hb = 0;
                if (send_heartbeat) { heartbeat(request); hb = heartbeat_sequence; }
                else {
                    identity(request); request["upload_uptime_ms"] = now_ms();
                    auto readings = request["readings"].to<JsonArray>();
                    for (size_t i=0; i<count; ++i) add_reading(readings.add<JsonObject>(), batch[i]);
                }
                Result r = post(send_heartbeat?"/api/v1/ingest/heartbeat":"/api/v1/ingest/readings", request, response);
                bool ok = r.code==200 && r.json && (send_heartbeat ?
                    response_identity(response.as<JsonVariantConst>(), KAELIX_GATEWAY_ID, session) && response.size()==5 &&
                    response["heartbeat_seq"].is<uint32_t>() && response["heartbeat_seq"].as<uint32_t>()==hb :
                    acknowledgements_valid(response.as<JsonVariantConst>(), KAELIX_GATEWAY_ID, session, batch, count));
                if (send_heartbeat) next_heartbeat = now_ms()+30000; // não guardar backlog de snapshots
                if (ok) {
                    if (!send_heartbeat) { lock(); queue.pop(count); acknowledged += count; unlock(); }
                    logging::log("gateway", "INFO", send_heartbeat?"HEARTBEAT":"UPLOAD", "confirmed=%u", send_heartbeat?1U:static_cast<unsigned>(count));
                    backoff=1000; next_upload=now_ms()+250;
                } else {
                    lock(); ++http_failures; unlock();
                    logging::log("gateway", "WARN", "HTTP", "status=%d confirmation=invalid; events retained", r.code);
                    if (r.code==401 || r.code==403 || r.code==404 || r.code==405 || r.code==301 || r.code==302 || r.code==307 || r.code==308) {
                        blocked=true;
                        logging::log("gateway", "ERROR", "CONFIG", "uploads paused until configuration is corrected and gateway rebooted");
                    } else if (r.code==400 || r.code==409 || r.code==413 || r.code==422) {
                        if (!send_heartbeat) {
                            lock();
                            dropped += quarantine_size;
                            quarantine_size=count;
                            for(size_t i=0;i<count;++i) quarantine[i]=batch[i];
                            queue.pop(count); unlock();
                            logging::log("gateway", "ERROR", "QUARANTINE", "events=%u first_seq=%lu status=%d; retained in RAM until reboot or replacement", static_cast<unsigned>(count), (unsigned long)batch[0].sequence, r.code);
                        } else blocked=true;
                    }
                    if (r.code>=400 && r.code<500 && r.code!=408 && r.code!=429 &&
                        r.code!=400 && r.code!=409 && r.code!=413 && r.code!=422) blocked=true;
                    next_upload=now_ms()+std::max<uint64_t>(r.retry_ms,backoff)+(esp_random()%1000);
                    backoff=std::min<uint32_t>(60000,backoff*2);
                }
            }
        }
        if (online && (clock_ready.load() || String(KAELIX_API_BASE_URL).startsWith("http://")) && now_ms()>=next_logs) {
            bool ok=send_logs();
            logs_backoff=ok?5000:std::min<uint32_t>(60000,logs_backoff*2);
            next_logs=now_ms()+logs_backoff;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
}
void begin() {
    logging::set_sink(capture_log);
    mutex = xSemaphoreCreateMutex();
    if (!mutex) { logging::log("gateway", "ERROR", "NET", "mutex allocation failed; serial-only mode"); return; }
    create_session();
    String base(KAELIX_API_BASE_URL);
    enabled = strlen(KAELIX_WIFI_SSID) && strlen(KAELIX_GATEWAY_TOKEN) && uuid(KAELIX_GATEWAY_ID) &&
        !base.endsWith("/") &&
        ((base.startsWith("https://") && strlen(KAELIX_TLS_CA_PEM)) ||
         (KAELIX_ALLOW_HTTP_LAN && base.startsWith("http://")));
    if (!enabled) { logging::log("gateway", "WARN", "CONFIG", "internet disabled; configure include/gateway_config.local.h"); return; }
    if (xTaskCreate(worker, "gateway-http", 12288, nullptr, 1, nullptr) != pdPASS) {
        enabled=false; logging::log("gateway", "ERROR", "NET", "worker allocation failed; serial-only mode");
    } else logging::log("gateway", "INFO", "NET", "gateway=%s session=%s; RAM queue=64 batch=8", KAELIX_GATEWAY_ID, session);
}
void poll() {
    if (!mutex || !Serial.available()) return;
    if (Serial.read() != 'Q') return;
    Event snapshot[8];
    lock(); const size_t count=quarantine_size;
    for (size_t i=0; i<count; ++i) snapshot[i]=quarantine[i];
    const size_t pending=queue.size();
    const uint64_t lost=dropped, ack=acknowledged;
    unlock();
    logging::log("gateway", "INFO", "QUEUE", "pending=%u quarantine=%u acknowledged=%llu dropped=%llu", static_cast<unsigned>(pending), static_cast<unsigned>(count), (unsigned long long)ack, (unsigned long long)lost);
    for(size_t i=0;i<count;++i) {
        JsonDocument d; add_reading(d.to<JsonObject>(), snapshot[i]);
        String json; serializeJson(d,json);
        // String inteira: log() tem limite de 384 caracteres.
        logging::line("gateway", "INFO", "QUARANTINE", json.c_str());
    }
}
void radio_health(bool initialized) { if(mutex) { lock(); radio_initialized=initialized; unlock(); } }
void rejected() { if(mutex) { lock(); ++invalid; unlock(); } }
void spi_error() { if(mutex) { lock(); ++spi; unlock(); } }
void enqueue(const comms::LoraPacket& packet, float rssi, float snr) {
    if (!mutex) return;
    Event e{}; e.packet=packet; e.received_ms=now_ms(); e.rssi=rssi; e.snr=snr;
    timestamp(e.received_at, sizeof(e.received_at));
    lock(); ++valid;
    bool full = false;
    if (enabled) {
        if (next_event==UINT32_MAX || !std::isfinite(rssi) || !std::isfinite(snr)) { ++dropped; full=true; }
        else { e.sequence=++next_event; if (!queue.push(e)) { ++dropped; full=true; } }
    }
    unlock();
    if(full) logging::log("gateway", "ERROR", "QUEUE", "event dropped; queue full, invalid RF metadata or sequence exhausted");
}
}
