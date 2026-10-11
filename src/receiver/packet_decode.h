#pragma once
#include "../comms/lora.h"
#include "crc16.h"
#include <cmath>
#include <cstring>

namespace kaelix::receiver {
enum class DecodeResult { Ok, Length, Crc, Version, State, Measurement };

// Somente publica campos após conferir o quadro inteiro. NaN = não medido.
inline DecodeResult decode_packet(const uint8_t* bytes, size_t length,
                                  kaelix::comms::LoraPacket* out) {
    if (bytes == nullptr || out == nullptr || length != sizeof(*out)) return DecodeResult::Length;
    kaelix::comms::LoraPacket candidate{};
    std::memcpy(&candidate, bytes, sizeof(candidate));
    if (kaelix::comms::crc16_ccitt(bytes, offsetof(kaelix::comms::LoraPacket, crc)) != candidate.crc)
        return DecodeResult::Crc;
    if (candidate.version != kaelix::comms::PACKET_VERSION) return DecodeResult::Version;
    if (candidate.state > static_cast<uint8_t>(kaelix::MachineState::Unknown)) return DecodeResult::State;
    if (std::isinf(candidate.rms) || std::isinf(candidate.temperature_c) ||
        (std::isfinite(candidate.rms) && candidate.rms < 0.0f) ||
        (candidate.state != static_cast<uint8_t>(kaelix::MachineState::Unknown) && std::isnan(candidate.rms)))
        return DecodeResult::Measurement;
    *out = candidate;
    return DecodeResult::Ok;
}
}
