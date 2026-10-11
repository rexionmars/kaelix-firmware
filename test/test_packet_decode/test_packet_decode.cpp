#include <unity.h>
#include <cmath>
#include "../../src/receiver/packet_decode.h"

void setUp() {}
void tearDown() {}
// Fixture independente: Python struct <BIIBBffH + binascii.crc_hqx(..., 0xffff).
const uint8_t golden[] = {0x02, 0x78, 0x56, 0x34, 0x12, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0xC0, 0x7F, 0x92, 0xE4};
const uint8_t unsupported[] = {0x01, 0x78, 0x56, 0x34, 0x12, 0x09, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0xC0, 0x7F, 0x92, 0x96};

void test_decode_independent_v2_frame_and_missing_temperature() {
    kaelix::comms::LoraPacket packet{};
    TEST_ASSERT_EQUAL(kaelix::receiver::DecodeResult::Ok,
        kaelix::receiver::decode_packet(golden, sizeof(golden), &packet));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, packet.device_id);
    TEST_ASSERT_EQUAL_UINT32(9, packet.boot_count);
    TEST_ASSERT_EQUAL_UINT8(2, packet.state);
    TEST_ASSERT_EQUAL_FLOAT(0.125f, packet.rms);
    TEST_ASSERT_TRUE(std::isnan(packet.temperature_c));
}
void test_corruption_does_not_publish_packet() {
    uint8_t damaged[sizeof(golden)];
    std::memcpy(damaged, golden, sizeof(golden));
    damaged[11] ^= 1U;
    kaelix::comms::LoraPacket packet{};
    packet.device_id = 99;
    TEST_ASSERT_EQUAL(kaelix::receiver::DecodeResult::Crc,
        kaelix::receiver::decode_packet(damaged, sizeof(damaged), &packet));
    TEST_ASSERT_EQUAL_UINT32(99, packet.device_id);
}
void test_truncated_and_unsupported_frames_are_rejected() {
    kaelix::comms::LoraPacket packet{};
    TEST_ASSERT_EQUAL(kaelix::receiver::DecodeResult::Length,
        kaelix::receiver::decode_packet(golden, sizeof(golden)-1, &packet));
    TEST_ASSERT_EQUAL(kaelix::receiver::DecodeResult::Version,
        kaelix::receiver::decode_packet(unsupported, sizeof(unsupported), &packet));
}
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_decode_independent_v2_frame_and_missing_temperature);
    RUN_TEST(test_corruption_does_not_publish_packet);
    RUN_TEST(test_truncated_and_unsupported_frames_are_rejected);
    return UNITY_END();
}
