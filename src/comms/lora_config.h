#pragma once
#include <cstdint>

// Uma configuração de enlace compartilhada por transmissor e receptor.
namespace kaelix::comms {
inline constexpr float LORA_FREQUENCY_MHZ = 433.0f;
inline constexpr float LORA_BANDWIDTH_KHZ = 125.0f;
inline constexpr uint8_t LORA_SPREADING_FACTOR = 9U;
inline constexpr uint8_t LORA_CODING_RATE = 5U; // 4/5
inline constexpr uint8_t LORA_SYNC_WORD = 0x12U;
inline constexpr uint16_t LORA_PREAMBLE_SYMBOLS = 8U;
}
