# Device — ESP32-C3 Super Mini

Primeira etapa da inversão: o C3 passa de receptor a transmissor de
vibração. A migração do S3 para gateway é uma etapa posterior.

## Ligações atuais

Use os números GPIO, não a posição física nos headers. Ligue com a
alimentação desligada. MPU e rádio compartilham o GND do C3.

| Módulo | Sinal | C3 |
|---|---|---|
| MPU6050/MPU6515 | VCC | 3V3 |
| MPU | GND, AD0 | GND |
| MPU | SDA | GPIO20 |
| MPU | SCL | GPIO21 |
| MPU | INT, XDA, XCL | sem conexão |
| RA-02 | VCC | 3V3 |
| RA-02 | todos os GND | GND |
| RA-02 | SCK | GPIO4 |
| RA-02 | MISO | GPIO5 |
| RA-02 | MOSI | GPIO6 |
| RA-02 | NSS | GPIO7 |
| RA-02 | RESET | GPIO3 |
| RA-02 | DIO0 | GPIO1 |

Antena de 433 MHz conectada ao RA-02. USB nativo para alimentação,
gravação e logs; GPIO20/21 ficam dedicados ao I2C, sem UART0 externa.

## Firmware

```bash
pio run -e esp32-c3-device
pio run -e esp32-c3-device -t upload --upload-port <porta>
pio device monitor -e esp32-c3-device --port <porta> -b 115200
```

O ambiente usa flash de 4 MB, sem PSRAM, sem Wi-Fi e sem gateway HTTP.
A pinagem do transmissor está em `src/device_pins.h`.
O ambiente antigo `esp32-c3-receiver` permanece disponível para reprodução.

O perfil atual reutiliza o ciclo de bancada: lê 512 amostras do eixo Z a
1 kHz pelo FIFO, remove a média e calcula RMS (g), curtose, fator de crista
e frequência dominante (Hz). Aceita identidade 0x68 (MPU6050) ou 0x74
(MPU6515), configura escala de ±4 g e DLPF de 184 Hz.

Transmite pacote LoRa v2 de 21 bytes (433 MHz, SF9, BW125, CR4/5,
sync 0x12), com potência de bancada de 2 dBm. Após cada tentativa, coloca
o rádio em sono e espera 10 s; o período completo inclui aquisição e TX.
`[PHASE] tx OK` confirma a conclusão local da transmissão, sem ACK.

O pacote contém ID, sequência, estado, diagnóstico, RMS e temperatura
ausente (NaN), além de versão/CRC. Os demais indicadores são exibidos no
serial. Como no perfil anterior de bancada, o estado é `UNKNOWN` e o modelo
não é usado para declarar normalidade/anomalia sem validação de campo.

Não há NTC, medição de bateria, load switch nem deep sleep nesta montagem.
O GPIO5 é MISO: não pode acionar o corte de energia previsto para a PCB S3.
O perfil C3 bloqueia configuração com NTC ou ciclo de produção até que haja
uma pinagem e alimentação próprias. A autonomia da PCB S3 não se aplica.

## Conferência no serial

- `BOOT`: identifica C3 e mostra a pinagem.
- `IMU`: endereço, identidade e configuração do sensor.
- `[PHASE] vib_init OK` e `[PHASE] vib_read OK`: inicialização e aquisição.
- `MEASURE`: indicadores calculados a partir da janela válida.
- `version=0x12`, `[PHASE] rf_init OK`: rádio identificado e configurado.
- `[PHASE] tx OK`: transmissão concluída pelo rádio.

Falhas de aquisição preservam o diagnóstico e RMS inválido no pacote;
não são convertidas em uma leitura de vibração zero.
