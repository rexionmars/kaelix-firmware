// Diagnóstico isolado, somente leitura: sem RadioLib, sensor ou transmissão.
#include <Arduino.h>
#include <SPI.h>

namespace {
#if defined(KAELIX_PROBE_C3)
constexpr uint8_t CS_PIN = 7;
constexpr uint8_t MOSI_PIN = 6;
constexpr uint8_t SCK_PIN = 4;
constexpr uint8_t MISO_PIN = 5;
constexpr uint8_t RESET_PIN = 3;
#else
constexpr uint8_t CS_PIN = 10;
constexpr uint8_t MOSI_PIN = 11;
constexpr uint8_t SCK_PIN = 12;
constexpr uint8_t MISO_PIN = 13;
constexpr uint8_t RESET_PIN = 21;
#endif
constexpr uint8_t VERSION_REG = 0x42;
SPIClass probe_spi(FSPI);

void reset_radio() {
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, LOW);
    delay(2);
    digitalWrite(RESET_PIN, HIGH);
    delay(20);
}

// SPI modo 0, MSB primeiro: MOSI muda com clock baixo, MISO é
// amostrado após a borda de subida. Cada meia fase dura >=10 us.
uint8_t software_transfer(uint8_t output) {
    uint8_t input = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        digitalWrite(MOSI_PIN, (output & 0x80U) != 0U ? HIGH : LOW);
        delayMicroseconds(10);
        digitalWrite(SCK_PIN, HIGH);
        delayMicroseconds(10);
        input = static_cast<uint8_t>((input << 1U) | (digitalRead(MISO_PIN) != LOW ? 1U : 0U));
        digitalWrite(SCK_PIN, LOW);
        output <<= 1U;
    }
    return input;
}

uint8_t software_version() {
    digitalWrite(CS_PIN, LOW);
    software_transfer(VERSION_REG); // bit 7=0: leitura
    const uint8_t version = software_transfer(0x00);
    digitalWrite(CS_PIN, HIGH);
    return version;
}

uint8_t hardware_version(uint32_t frequency) {
    probe_spi.beginTransaction(SPISettings(frequency, MSBFIRST, SPI_MODE0));
    digitalWrite(CS_PIN, LOW);
    probe_spi.transfer(VERSION_REG);
    const uint8_t version = probe_spi.transfer(0x00);
    digitalWrite(CS_PIN, HIGH);
    probe_spi.endTransaction();
    return version;
}
}

void setup() {
    Serial.begin(115200);
    delay(1500);
    Serial.printf("[probe] READ ONLY CS=%u MOSI=%u SCK=%u MISO=%u RST=%u expected=0x12\n",
                  CS_PIN, MOSI_PIN, SCK_PIN, MISO_PIN, RESET_PIN);
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    pinMode(SCK_PIN, OUTPUT);
    digitalWrite(SCK_PIN, LOW);
    pinMode(MOSI_PIN, OUTPUT);
    digitalWrite(MOSI_PIN, LOW);
    pinMode(MISO_PIN, INPUT);
    reset_radio();
    for (uint8_t i = 0; i < 5; ++i) {
        const uint8_t version = software_version();
        Serial.printf("[probe] gpio_spi version=0x%02X\n", version);
        delay(20);
    }
    probe_spi.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    if (probe_spi.bus() == nullptr) {
        Serial.println("[probe] hardware_spi BUS_INIT_FAILED");
        return;
    }
    reset_radio();
    for (uint8_t i = 0; i < 5; ++i) {
        const uint8_t slow = hardware_version(100000);
        const uint8_t fast = hardware_version(1000000);
        Serial.printf("[probe] hardware_spi 100k=0x%02X 1M=0x%02X\n", slow, fast);
        delay(20);
    }
    probe_spi.end();
    Serial.println("[probe] done; RESET/EN repete o teste");
    Serial.flush();
}

void loop() {
    delay(1000);
}
