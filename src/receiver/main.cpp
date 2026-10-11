#include <Arduino.h>
#include "../serial_log.h"
#include <RadioLib.h>
#include <SPI.h>
#include "../comms/lora_config.h"
#include "packet_decode.h"

namespace {
constexpr uint8_t SCK_PIN = 4;
constexpr uint8_t MISO_PIN = 5;
constexpr uint8_t MOSI_PIN = 6;
constexpr uint8_t CS_PIN = 7;
constexpr uint8_t RESET_PIN = 3;
constexpr uint8_t DIO0_PIN = 1;
Module module(CS_PIN, DIO0_PIN, RESET_PIN, RADIOLIB_NC,
              SPI, SPISettings(100000, MSBFIRST, SPI_MODE0));
SX1278 radio(&module);
volatile bool packet_ready = false;
bool receiving = false;
uint32_t last_attempt = 0;
uint32_t last_poll = 0;
uint32_t last_health = 0;
uint32_t received_packets = 0;
uint32_t rejected_packets = 0;
uint32_t polled_packets = 0;

void IRAM_ATTR on_packet() { packet_ready = true; }

bool start_listening() {
    packet_ready = false;
    const int16_t result = radio.startReceive();
    if (result != RADIOLIB_ERR_NONE) kaelix::logging::log("gateway", "ERROR", "RADIO", "start_rx error=%d", result);
    return result == RADIOLIB_ERR_NONE;
}

bool initialize_radio() {
    namespace cfg = kaelix::comms;
    const int16_t result = radio.begin(cfg::LORA_FREQUENCY_MHZ, cfg::LORA_BANDWIDTH_KHZ,
        cfg::LORA_SPREADING_FACTOR, cfg::LORA_CODING_RATE, cfg::LORA_SYNC_WORD,
        2, cfg::LORA_PREAMBLE_SYMBOLS);
    kaelix::logging::log("gateway", result == RADIOLIB_ERR_NONE ? "INFO" : "ERROR", "RADIO", "rf_init=%d version=0x%02X", result,
                  static_cast<unsigned>(static_cast<uint16_t>(radio.getChipVersion())));
    if (result != RADIOLIB_ERR_NONE) return false;
    kaelix::logging::log("gateway", "INFO", "CONFIG", "config frf=%02X%02X%02X modem1=0x%02X modem2=0x%02X sync=0x%02X",
        module.SPIreadRegister(0x06), module.SPIreadRegister(0x07),
        module.SPIreadRegister(0x08), module.SPIreadRegister(0x1D),
        module.SPIreadRegister(0x1E), module.SPIreadRegister(0x39));
    radio.setDio0Action(on_packet, RISING);
    return start_listening();
}

void print_packet(const kaelix::comms::LoraPacket& packet, float rssi, float snr) {
    char temperature[24];
    if (std::isnan(packet.temperature_c)) snprintf(temperature, sizeof(temperature), "NA");
    else snprintf(temperature, sizeof(temperature), "%.2f C", packet.temperature_c);
    kaelix::logging::log("gateway", "INFO", "RX", "RX id=%08lX seq=%lu state=%s diag=%s rms=%.6f g temp=%s rssi=%.1f dBm snr=%.1f dB crc=OK",
        static_cast<unsigned long>(packet.device_id), static_cast<unsigned long>(packet.boot_count),
        kaelix::machine_state_to_string(static_cast<kaelix::MachineState>(packet.state)),
        kaelix::status_to_string(static_cast<kaelix::Status>(packet.diag)), packet.rms,
        temperature, rssi, snr);
}
}

void setup() {
    Serial.begin(115200);
    // Não depender do computador estar conectado para começar a escutar.
    delay(1500);
    kaelix::logging::log("gateway", "INFO", "BOOT", "C3 RX SCK=4 MISO=5 MOSI=6 CS=7 RST=3 DIO0=1 packet=v2/21B");
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    if (SPI.bus() == nullptr) {
        kaelix::logging::log("gateway", "ERROR", "SPI", "SPI_INIT_FAILED");
        return;
    }
    receiving = initialize_radio();
    last_attempt = millis();
    if (receiving) kaelix::logging::log("gateway", "INFO", "RADIO", "listening 433MHz SF9 BW125 CR4/5 sync=0x12");
}

void loop() {
    if (!receiving) {
        if (SPI.bus() != nullptr && millis() - last_attempt >= 5000U) {
            last_attempt = millis();
            receiving = initialize_radio();
            if (receiving) kaelix::logging::log("gateway", "INFO", "RADIO", "listening");
        }
        delay(10);
        return;
    }
    // Consultar RxDone também por SPI: a recepção não fica dependente
    // de uma borda DIO0 ter sido observada pelo callback.
    const uint32_t now = millis();
    bool from_irq = packet_ready;
    uint16_t flags = 0;
    if (!from_irq && now - last_poll < 10U) {
        delay(1);
        return;
    }
    last_poll = now;
    flags = radio.getIRQFlags();
    if (now - last_health >= 5000U) {
        last_health = now;
        const uint8_t version = module.SPIreadRegister(0x42);
        kaelix::logging::log("gateway", version == 0x12 ? "INFO" : "ERROR", "HEALTH", "health version=0x%02X op=0x%02X irq=0x%02X dio0=%d rssi=%.1f rx=%lu reject=%lu poll=%lu",
            version, module.SPIreadRegister(0x01), static_cast<unsigned>(flags),
            digitalRead(DIO0_PIN), radio.getRSSI(false),
            static_cast<unsigned long>(received_packets),
            static_cast<unsigned long>(rejected_packets),
            static_cast<unsigned long>(polled_packets));
        if (version != 0x12) {
            kaelix::logging::log("gateway", "ERROR", "SPI", "SPI_VERSION_LOST; retry initialization");
            receiving = false;
            last_attempt = now;
            return;
        }
    }
    if ((flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) == 0U) {
        if (from_irq) {
            packet_ready = false;
            kaelix::logging::log("gateway", "WARN", "IRQ", "DIO0 event without RxDone irq=0x%02X",
                static_cast<unsigned>(flags));
        }
        delay(1);
        return;
    }
    // Flags lidos por SPI não são prova de pacote quando o barramento
    // perde o identificador. Conferir novamente antes de acessar o FIFO.
    const uint8_t version_first = module.SPIreadRegister(0x42);
    const uint8_t version_second = module.SPIreadRegister(0x42);
    if (version_first != 0x12 || version_second != 0x12) {
        kaelix::logging::log("gateway", "ERROR", "SPI", "SPI_UNSTABLE irq=0x%02X versions=0x%02X/0x%02X; retry initialization",
            static_cast<unsigned>(flags), version_first, version_second);
        packet_ready = false;
        receiving = false;
        last_attempt = now;
        return;
    }
    // Reavaliar após a leitura SPI: a ISR pode ter ocorrido nesse intervalo.
    from_irq = from_irq || packet_ready;
    packet_ready = false;
    if (!from_irq) ++polled_packets;
    kaelix::logging::log("gateway", "DEBUG", "IRQ", "rx_event via=%s irq=0x%02X dio0=%d",
        from_irq ? "DIO0" : "SPI", static_cast<unsigned>(flags), digitalRead(DIO0_PIN));
    uint8_t bytes[255]{};
    const size_t length = radio.getPacketLength();
    if (length == 0U || length > sizeof(bytes)) {
        ++rejected_packets;
        kaelix::logging::log("gateway", "WARN", "REJECT", "reject length=%u", static_cast<unsigned>(length));
        kaelix::logging::log("gateway", "DEBUG", "SPI", "after_reject version=0x%02X op=0x%02X irq=0x%02X",
            module.SPIreadRegister(0x42), module.SPIreadRegister(0x01),
            module.SPIreadRegister(0x12));
        receiving = start_listening();
        return;
    }
    const int16_t result = radio.readData(bytes, length);
    if (result == RADIOLIB_ERR_NONE) {
        const float rssi = radio.getRSSI();
        const float snr = radio.getSNR();
        kaelix::comms::LoraPacket packet{};
        const auto decoded = kaelix::receiver::decode_packet(bytes, length, &packet);
        if (decoded == kaelix::receiver::DecodeResult::Ok) {
            ++received_packets;
            print_packet(packet, rssi, snr);
        } else {
            ++rejected_packets;
            kaelix::logging::log("gateway", "WARN", "REJECT", "reject decode=%u length=%u",
                static_cast<unsigned>(decoded), static_cast<unsigned>(length));
        }
    } else {
        ++rejected_packets;
        kaelix::logging::log("gateway", "WARN", "REJECT", "read error=%d", result);
        kaelix::logging::log("gateway", "DEBUG", "SPI", "after_read_error version=0x%02X op=0x%02X",
            module.SPIreadRegister(0x42), module.SPIreadRegister(0x01));
    }
    receiving = start_listening();
}
