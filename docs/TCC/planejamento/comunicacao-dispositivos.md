# Comunicação entre os dispositivos e monitoramento de vários motores

Registro para análise posterior — 11/10/2026.

## Caminho dos dados

O ESP32-C3 Super Mini com o segundo módulo LoRa será a central receptora, também chamada de gateway.

```text
Sensor no motor → ESP32-S3 → LoRa pelo ar → LoRa + C3
                                                    ↓
                                          Validado: monitor serial
                                          A validar: Wi-Fi → API → banco → painel
```

O **S3 instalado no motor** coleta a vibração, calcula os indicadores e envia um pacote pequeno de **21 bytes**. Ele contém identificação do dispositivo, sequência, estado, diagnóstico, RMS e temperatura quando disponível. As amostras completas de vibração não são enviadas atualmente.

O **C3 recebe o pacote**, verifica sua integridade e mostra os dados pelo USB no computador. O caminho validado em bancada termina no monitor serial. A API Go com PostgreSQL e a base desktop já existem; o firmware agora inclui transporte Wi-Fi/HTTPS, ainda pendente de configuração e teste físico. A recepção foi validada em bancada com pacotes v2 e CRC correto; as falhas SPI anteriores ainda precisam ser acompanhadas em ensaios maiores.

Para testar o envio pela rede, seguir a [configuração do gateway C3](../../../src/receiver/INTERNET.md). O cliente de API no desktop é uma etapa separada.

## Como seria para dez motores

```text
Motor 1  → S3 + sensor + LoRa ─┐
Motor 2  → S3 + sensor + LoRa ─┤
...                           ├→ Um C3 + LoRa → histórico/painel
Motor 10 → S3 + sensor + LoRa ─┘
```

Cada motor precisa do seu conjunto de monitoramento. **Não é necessário um receptor para cada motor**: um receptor pode atender os dez, desde que estejam dentro da cobertura e as transmissões sejam organizadas. Essa capacidade deve ser validada em testes com vários dispositivos.

O campo `device_id` permite distinguir os dispositivos. No painel, cadastraríamos, por exemplo, `12345678 → Motor da bomba 1`.

O cuidado é que **esse receptor escuta um pacote por vez**. Se dois dispositivos transmitirem juntos, pode haver perda. Precisamos distribuir os envios no tempo e acompanhar a sequência e a última leitura de cada motor.

Atualmente também não há confirmação de recebimento: `tx OK` no S3 significa que ele transmitiu; a linha `RX` no C3 confirma que o pacote chegou.

## Próximos passos para o TCC

1. Validar a comunicação **um motor → receptor** nas placas.
2. Salvar as medições recebidas com identificação do dispositivo e horário de recepção.
3. Ampliar para vários dispositivos e medir alcance, perdas e regularidade das leituras.
4. Integrar o histórico a um painel e associar cada dispositivo ao motor monitorado.

Nesta etapa de bancada, sem NTC e com o modelo ainda não validado para o motor, `temp=NA` e `state=UNKNOWN` são esperados. Receber os pacotes valida a comunicação; a classificação de anomalias exige validação própria.

## Referência de implementação

As ligações do RA-02 ao C3 Super Mini e os comandos de upload estão no [guia do receptor](../../../src/receiver/README.md).

A evolução para gateway pela internet e desktop Wails está na [proposta de arquitetura](arquitetura-desktop-internet.md).
