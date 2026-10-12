# Gateway C3 → API Go

O ambiente `esp32-c3-receiver` recebe LoRa e envia leituras pelo Wi-Fi para o contrato JSON v1. O rádio permanece na tarefa principal; uma tarefa FreeRTOS separada cuida da rede. Não há alterações no pacote do S3 nem nos pinos do C3.

## Configuração local

Editar `include/gateway_config.local.h`, já criado com campos vazios e ignorado pelo Git. Se necessário, copiar `include/gateway_config.example.h`.

| Define | Valor |
|---|---|
| `KAELIX_WIFI_SSID` | Rede Wi-Fi 2,4 GHz |
| `KAELIX_WIFI_PASSWORD` | Senha da rede |
| `KAELIX_API_BASE_URL` | Origem HTTPS, sem barra final e sem `/api/v1` |
| `KAELIX_GATEWAY_ID` | UUID em minúsculas cadastrado no servidor |
| `KAELIX_GATEWAY_TOKEN` | Credencial Bearer entregue pelo cadastro do gateway |
| `KAELIX_TLS_CA_PEM` | Certificado PEM da CA que valida o servidor |
| `KAELIX_ALLOW_HTTP_LAN` | `0` normalmente; `1` permite HTTP explícito para teste em LAN |

Para um certificado com várias linhas, usar literal C++ `R"PEM(...conteúdo PEM...)PEM"`. HTTPS sempre verifica a CA e aguarda sincronização NTP; não existe chamada a `setInsecure()`. O token e a senha não são impressos nos logs. O arquivo local entra no binário, portanto o binário também contém as credenciais.

Sem configuração completa, o gateway registra `[CONFIG] internet disabled` e continua recebendo pelo LoRa/serial. Falha na alocação da tarefa também mantém a recepção local.

O endereço `127.0.0.1` não alcança a API do computador a partir do C3. Para teste em LAN, a API/proxy deve estar acessível pelo IP do computador e firewall. O Compose do servidor publica apenas em loopback por padrão. HTTP não fornece confidencialidade ao token; usar somente em bancada de rede controlada. Para internet, configurar HTTPS acessível e a CA correspondente.

## Compilar e gravar

Depois de preencher a configuração e fechar o monitor serial:

```sh
pio run -e esp32-c3-receiver
pio run -e esp32-c3-receiver -t upload --upload-port /dev/cu.usbmodem2101
pio device monitor -e esp32-c3-receiver --port /dev/cu.usbmodem2101 -b 115200
```

O nome da porta pode mudar ao reconectar; conferir com `pio device list`. Os ambientes de probe continuam somente com o diagnóstico SPI.

## Entrega e recuperação

- Fila RAM de 64 eventos; lote de até 8, preservando a ordem. Fila cheia descarta a leitura nova e incrementa `queue_dropped`.
- UUID v4 novo por boot; sequência de evento uint32. Uma leitura retransmitida mantém identidade, valores e horário original de recepção. `upload_uptime_ms` muda a cada tentativa.
- NaN de RMS/temperatura vira `null`; `UNKNOWN` é preservado. Horário UTC fica `null` até uma sincronização NTP. O horário não é preenchido retroativamente.
- Só remove o lote após HTTP 200 e confirmação exata: versão, gateway, sessão, horário UTC, todas as sequências únicas e status `stored`/`duplicate`. Confirmação incompleta ou inválida mantém os eventos.
- Rede/timeout/5xx/429: retentativa com atraso exponencial de 1 a 60 segundos e jitter; `Retry-After` reconhecido também é respeitado. Conexão e operações HTTP usam timeout de 5 segundos; handshake TLS tem seu próprio limite de 5 segundos.
- 401/403, endpoint inexistente, método recusado ou redirecionamento: pausa uploads até corrigir a configuração e reiniciar; a fila continua recebendo até encher. Nenhum redirecionamento encaminha o token a outro destino.
- 400/409/413/422 em leituras: lote vai para quarentena RAM de até 8 eventos, liberando a fila. Substituir uma quarentena conta os eventos anteriores como descartados. Erro permanente de heartbeat pausa uploads.
- Heartbeat atual a cada 30 segundos conectado, com saúde de rádio/rede, fila, heap e contadores. Não acumula backlog de heartbeat. Cada tentativa de snapshot novo usa sequência nova; nenhuma confirmação de heartbeat remove leituras.
- A sequência de leitura não dá a volta: após uint32 máximo, leituras novas são descartadas até reboot para evitar reutilização de identidade.

Enviar `Q` maiúsculo pelo monitor serial mostra ocupação da fila, contadores e os JSONs da quarentena; não apaga eventos. Habilitar `send_on_enter` no monitor se precisar enviar a letra com Enter.

**Limitação desta versão:** fila e quarentena não persistem em flash. Desligar/reiniciar perde eventos pendentes. Ainda é necessário testar Wi-Fi/HTTPS no hardware, perda de rede, consumo de heap e o percurso real até PostgreSQL/desktop; compilação e testes nativos não comprovam essa integração.

## Verificação

```sh
pio test -e native -f test_gateway_delivery -f test_packet_decode
pio run -e esp32-c3-receiver
```

Os testes cobrem confirmação fora de ordem, duplicatas, confirmações incompletas/repetidas/de sessão errada, FIFO cheia enquanto um lote aguarda confirmação, nulos, estado anômalo e sequência uint32.

Verificação realizada em 11/10/2026: 83 testes nativos passaram; um lote de oito leituras produzido pelo serializador C++ foi validado pelo JSON Schema v1. Compilações do receptor C3, probe C3 e bancada S3 passaram. Uma compilação separada, com configuração técnica não vazia e sem credenciais reais, também passou com Wi-Fi/HTTPS incluídos no binário (73,4% da partição de aplicação). Os percentuais estáticos não representam o heap livre durante TLS; isso deve ser medido no ensaio físico.

Primeiro ensaio físico em LAN (11/10/2026): firmware gravado no C3, API `/readyz` respondeu 200, relógio NTP sincronizou e o gateway registrou `[HEARTBEAT] confirmed=1`. Esse ensaio usou HTTP de bancada, não HTTPS. O rádio permaneceu em recepção com `version=0x12`, mas a janela observada não recebeu pacotes LoRa (`rx=0`); envio de leituras reais e ensaios de reconexão ainda precisam ser confirmados.

## Diagnóstico remoto para o desktop

O logger do C3 agora espelha BOOT, RADIO, HEALTH, SPI, RX, UPLOAD, HTTP e demais eventos Kaelix em uma fila separada de 64 linhas. Envia lotes de até oito linhas a cada cinco segundos por `/api/v1/ingest/logs`, com a mesma credencial e sessão do gateway. A fila de diagnóstico não confirma nem retira medições. Cada linha tem sequência, uptime, nível, evento, mensagem e indicação de truncamento. Falhas de rede preservam o buffer até sua capacidade; excedentes e reboot podem perder logs. Não chamar o logger de ISR.

No desktop atualizado, abrir **Dev mode** na barra superior e selecionar o gateway. A API retém até 5.000 linhas por gateway; a tela permite filtrar, pausar e exportar. Consultas são autenticadas com a conta de usuário da API, sem expor o token do gateway no frontend. O contrato adicional está em [gateway-logs-v1](../../docs/contracts/gateway-logs-v1/README.md).

É necessário gravar novamente o firmware do C3 uma vez via USB. Depois disso, os logs são enviados pelo Wi-Fi e o USB deixa de ser necessário para acompanhá-los. O pacote do S3 permanece v2/21 bytes: inclui valores/diagnóstico nas recepções RX, mas não transporta logs internos do S3 nem mensagens de ROM.
