#pragma once
#include "../comms/lora.h"
namespace kaelix::receiver::internet {
void begin();
// Comando serial Q lista a quarentena, sem apagar eventos.
void poll();
void radio_health(bool initialized);
void rejected();
void spi_error();
void enqueue(const comms::LoraPacket& packet, float rssi, float snr);
}
