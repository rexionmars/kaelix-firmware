#include <unity.h>
#include "../../src/receiver/delivery.h"
#include <limits>
using namespace kaelix::receiver;
void setUp() {}
void tearDown() {}
const char* gateway="5ef49647-9449-4f47-8d1d-c06611c3ba81";
const char* session="17162ec5-4fe1-4dac-a871-879120324a57";
void reply(JsonDocument& d) {
    d["schema_version"]=1; d["gateway_id"]=gateway; d["gateway_session_id"]=session;
    d["server_received_at"]="2026-10-11T19:00:00Z";
    auto a=d["acknowledged"].to<JsonArray>();
    auto first=a.add<JsonObject>(); first["event_seq"]=2; first["status"]="duplicate";
    auto second=a.add<JsonObject>(); second["event_seq"]=1; second["status"]="stored";
}
void test_confirmed_batch_accepts_out_of_order_duplicates() {
    Event e[2]{};e[0].sequence=1;e[1].sequence=2; JsonDocument d; reply(d);
    TEST_ASSERT_TRUE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
}
void test_incomplete_repeated_foreign_confirmations_are_rejected() {
    Event e[2]{};e[0].sequence=1;e[1].sequence=2; JsonDocument d;reply(d);
    d["acknowledged"][0]["event_seq"]=1;
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    d["acknowledged"][0]["event_seq"]=3;
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    d["acknowledged"].as<JsonArray>().remove(0);
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
}
void test_wrong_session_schema_status_and_sequence_type_are_rejected() {
    Event e[2]{};e[0].sequence=1;e[1].sequence=2; JsonDocument d;reply(d);
    d["gateway_session_id"]="another";
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    reply(d);d["schema_version"]=2;
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    reply(d);d["acknowledged"][0]["status"]="accepted";
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    reply(d);d["acknowledged"][0]["event_seq"]="2";
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
}
void test_fifo_keeps_pending_batch_when_new_events_arrive() {
    EventQueue<3> q;Event e{};Event batch[3]{};
    e.sequence=1;TEST_ASSERT_TRUE(q.push(e));e.sequence=2;TEST_ASSERT_TRUE(q.push(e));
    TEST_ASSERT_EQUAL(2,q.snapshot(batch,2));
    e.sequence=3;TEST_ASSERT_TRUE(q.push(e));e.sequence=4;TEST_ASSERT_FALSE(q.push(e));
    TEST_ASSERT_EQUAL(3,q.size());TEST_ASSERT_EQUAL(1,batch[0].sequence);
    q.pop(2);TEST_ASSERT_TRUE(q.push(e));
    TEST_ASSERT_EQUAL(2,q.snapshot(batch,3));TEST_ASSERT_EQUAL(3,batch[0].sequence);TEST_ASSERT_EQUAL(4,batch[1].sequence);
}
void test_null_measurements_and_uint32_are_preserved() {
    Event e{};e.sequence=UINT32_MAX;e.packet.version=2;e.packet.boot_count=UINT32_MAX;
    e.packet.device_id=0xD6F61B44;e.packet.state=2;e.packet.diag=255;e.packet.crc=65535;
    e.packet.rms=std::numeric_limits<float>::quiet_NaN();e.packet.temperature_c=e.packet.rms;
    e.rssi=-55;e.snr=13.5;
    JsonDocument d;add_reading(d.to<JsonObject>(),e);
    TEST_ASSERT_EQUAL_STRING("UNKNOWN",d["state"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("D6F61B44",d["device_id"].as<const char*>());
    TEST_ASSERT_TRUE(d["rms_g"].isNull());TEST_ASSERT_TRUE(d["temperature_c"].isNull());
    TEST_ASSERT_TRUE(d["gateway_received_at"].isNull());TEST_ASSERT_EQUAL_UINT32(UINT32_MAX,d["device_sequence"].as<uint32_t>());
    TEST_ASSERT_EQUAL(13,d.size());
}
void test_anomaly_and_original_reception_time_survive_retries() {
    Event e{};e.packet.state=1;e.packet.rms=0.125;e.packet.temperature_c=25;
    strcpy(e.received_at,"2026-10-11T19:00:00Z");e.received_ms=12345;
    JsonDocument d;add_reading(d.to<JsonObject>(),e);
    TEST_ASSERT_EQUAL_STRING("ANOMALOUS",d["state"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING(e.received_at,d["gateway_received_at"].as<const char*>());
    TEST_ASSERT_EQUAL_FLOAT(0.125f,d["rms_g"].as<float>());
    TEST_ASSERT_EQUAL_UINT64(12345,d["gateway_rx_uptime_ms"].as<uint64_t>());
}
void test_confirmation_requires_valid_utc_and_known_fields() {
    Event e[2]{};e[0].sequence=1;e[1].sequence=2; JsonDocument d;reply(d);
    d["server_received_at"]="2026-02-30T19:00:00Z";
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    reply(d);d["unexpected"]=true;
    TEST_ASSERT_FALSE(acknowledgements_valid(d.as<JsonVariantConst>(),gateway,session,e,2));
    TEST_ASSERT_TRUE(valid_utc("2024-02-29T19:00:00.123Z"));
    TEST_ASSERT_FALSE(valid_utc("2026-10-11T25:00:00Z"));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_confirmed_batch_accepts_out_of_order_duplicates);
    RUN_TEST(test_incomplete_repeated_foreign_confirmations_are_rejected);
    RUN_TEST(test_wrong_session_schema_status_and_sequence_type_are_rejected);
    RUN_TEST(test_fifo_keeps_pending_batch_when_new_events_arrive);
    RUN_TEST(test_null_measurements_and_uint32_are_preserved);
    RUN_TEST(test_anomaly_and_original_reception_time_survive_retries);
    RUN_TEST(test_confirmation_requires_valid_utc_and_known_fields);
    return UNITY_END();
}
