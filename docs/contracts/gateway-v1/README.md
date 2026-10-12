# Contrato JSON — gateway C3 → API Go

Contrato v1 adotado pela API Go — 11/10/2026. O transporte JSON está implementado no firmware C3; a integração física com a API ainda precisa ser validada. JSON Schema: draft 2020-12. Os UUIDs dos exemplos são fixtures técnicas; serão substituídos pelo cadastro/sessão reais.

**Atualização:** a API Go adotou estes schemas e exemplos e corrigiu as divergências anteriores. A [revisão de compatibilidade](compatibilidade-api-go.md) registra a verificação e os próximos passos no C3.

## Endpoints e arquivos

| Operação | Endpoint | Schema da requisição | Schema da resposta |
|---|---|---|---|
| Enviar leituras | `POST /api/v1/ingest/readings` | [readings-request](readings-request.schema.json) | [readings-response](readings-response.schema.json) |
| Enviar saúde do gateway | `POST /api/v1/ingest/heartbeat` | [heartbeat-request](heartbeat-request.schema.json) | [heartbeat-response](heartbeat-response.schema.json) |
| Erro de qualquer endpoint | — | — | [error-response](error-response.schema.json) |

Exemplos completos estão em [examples](examples/): leitura, confirmação, confirmação de duplicata, heartbeat e erro. Não há nomes fictícios de motores; o contrato transporta identidades e dados técnicos.

Os dois endpoints exigem HTTPS, `Content-Type: application/json` e `Authorization: Bearer <credencial-do-gateway>`. O servidor vincula a credencial a um `gateway_id`; não aceita que o corpo escolha outro gateway. Credenciais não entram no JSON, nos exemplos ou nos logs. Limite inicial de corpo: 32 KiB. Não enviar números NaN/Infinity nem JSON comprimido nesta versão.

## Requisição de leituras

| Campo do lote | Regra |
|---|---|
| `schema_version` | Inteiro `1`, versão do JSON |
| `gateway_id` | UUID em minúsculas, estável e atribuído pelo cadastro da API |
| `gateway_session_id` | UUID novo gerado pelo C3 em cada boot; preservado em todos os reenvios daquela sessão |
| `upload_uptime_ms` | Tempo monotônico desde o boot no início dessa tentativa HTTP |
| `readings` | Entre 1 e 8 eventos de recepção |

| Campo de cada leitura | Regra |
|---|---|
| `event_seq` | Contador do gateway, de 1 até 4294967295, incrementado por leitura validada |
| `gateway_rx_uptime_ms` | Tempo monotônico da recepção; não pode superar `upload_uptime_ms` |
| `gateway_received_at` | UTC RFC 3339 terminado em `Z`, ou `null` se o relógio não estava sincronizado |
| `packet_version` | Inteiro `2`, versão LoRa, independente de `schema_version` |
| `device_id` | ID LoRa normalizado como 8 caracteres hexadecimais em maiúsculas, incluindo zeros à esquerda |
| `device_sequence` | Contador `boot_count` recebido no pacote, de 0 até 4294967295 |
| `state` | `NORMAL`, `ANOMALOUS` ou `UNKNOWN` |
| `diag_code` | Código numérico de diagnóstico, de 0 a 255; `0` é OK |
| `rms_g` | Número finito não negativo ou `null`; `null` só é permitido com `UNKNOWN` |
| `temperature_c` | Número finito ou `null`; ausência de NTC vira `null` |
| `rssi_dbm` | RSSI finito do pacote recebido, em dBm |
| `snr_db` | SNR finito do pacote recebido, em dB |
| `packet_crc16` | CRC do pacote LoRa recebido, de 0 a 65535 |

Todos os campos são obrigatórios, inclusive os que aceitam `null`. Campos desconhecidos são rejeitados na v1. Contadores monotônicos/uptime limitam-se a 9007199254740991 para manter representação inteira exata no TypeScript; contador de evento usa uint32 e deve iniciar nova sessão antes de ultrapassar o limite.

O C3 aceita apenas pacotes LoRa de 21 bytes com CRC válido antes de montar o JSON. `packet_crc16` é rastreabilidade do quadro, não uma assinatura de autenticidade. A API não deve reconstruir/revalidar o CRC a partir desse JSON: NaN virou `null` e a representação binária original não está incluída.

`diag_code` desconhecido deve ser preservado e exibido como código desconhecido, sem assumir OK. Não inventar um enum `ATTENTION` no protocolo: a interface pode apresentar `ANOMALOUS` como “Anomalia” ou outro rótulo apropriado, mas mantém o significado original. `UNKNOWN` nunca vira `NORMAL`.

### Validações além do JSON Schema

A implementação Go deve validar explicitamente:

1. Credencial compatível com `gateway_id`.
2. `event_seq` sem repetição no mesmo lote, mesmo que os objetos sejam diferentes.
3. `gateway_rx_uptime_ms <= upload_uptime_ms` para cada leitura.
4. Todos os números finitos, inclusive na desserialização/conversão interna.
5. JSON sem chaves duplicadas, sem dados extras após o objeto e dentro do limite de corpo.
6. Evento já persistido com conteúdo imutável igual, conforme regra de idempotência.

Não rejeitar um dispositivo apenas porque ainda não foi associado a um motor: persistir com sua identidade e permitir cadastro posterior. Aceitar códigos de diagnóstico de todo o intervalo uint8, mesmo que a tabela do servidor esteja desatualizada.

## Confirmação e idempotência

Chave única: **`(gateway_id, gateway_session_id, event_seq)`**. Não usar apenas ID do sensor + sequência: o S3 pode reiniciar a sequência após reset.

Proposta de armazenamento: colunas UUID para gateway/sessão, BIGINT para sequências/uptime, `device_id` como texto hexadecimal normalizado, timestamps UTC e valores opcionais com NULL. A chave de evento exige restrição UNIQUE no banco para suportar requisições concorrentes.

O lote é **atômico**: validar tudo antes, persistir em uma transação e confirmar apenas depois do commit. Erro de validação ou conflito em qualquer item não persiste novos itens desse lote. Um lote pode misturar eventos novos com duplicatas idênticas.

Resposta **HTTP 200**:

```json
{
  "schema_version": 1,
  "gateway_id": "5ef49647-9449-4f47-8d1d-c06611c3ba81",
  "gateway_session_id": "17162ec5-4fe1-4dac-a871-879120324a57",
  "server_received_at": "2026-10-11T19:00:00.000Z",
  "acknowledged": [
    { "event_seq": 1, "status": "stored" }
  ]
}
```

- `stored`: inserido e confirmado no banco.
- `duplicate`: mesma chave e mesmos campos da leitura já estavam persistidos; não inserir novamente.
- Mesma chave com qualquer campo imutável diferente: **HTTP 409 `EVENT_CONFLICT`**. Não sobrescrever a medição anterior.
- `upload_uptime_ms` é da tentativa, pode mudar no reenvio e não participa da comparação do conteúdo da leitura.
- Todos os campos dentro de cada leitura são imutáveis. Não preencher retroativamente `gateway_received_at` após sincronizar NTP.
- Na duplicata, preservar o `ingested_at` original da medição. `server_received_at` na resposta é o horário da tentativa atual, não o horário original de aquisição.

A resposta deve confirmar exatamente os eventos do lote, cada um uma vez; a ordem não é relevante. O C3 confere versão, gateway e sessão, e não aceita IDs extras/repetidos ou confirmação incompleta. Só remove a fila após uma resposta válida para o lote enviado. Resposta perdida, inválida ou timeout mantém o lote para reenvio com as mesmas identidades. Essa confirmação não é ACK LoRa para o S3.

Se um lote recebe erro permanente, mantê-lo indisponível para novos reenvios automáticos até tratamento: registrar o motivo em uma quarentena limitada e liberar a fila operacional, contando descartes se a quarentena encher. A implementação dessa quarentena deve definir capacidade e política; não entrar em repetição infinita de HTTP 422/409.

## Heartbeat

Proposta: enviar a cada 30 segundos quando houver conexão. O servidor registra o instante real de chegada; se não receber por 90 segundos, marca a conexão do gateway como sem atividade recente. Isso não indica falha mecânica do motor. Desktop offline, gateway sem atividade e sensor sem leitura são estados separados.

| Campo | Regra |
|---|---|
| `schema_version`, `gateway_id`, `gateway_session_id` | Mesmas regras do lote |
| `heartbeat_seq` | Sequência crescente por sessão; reenvio do mesmo heartbeat mantém a sequência e corpo |
| `uptime_ms` | Tempo desde o boot na criação do heartbeat |
| `sent_at` | UTC RFC 3339 ou `null` sem relógio sincronizado |
| `firmware_version` | Versão do build, entre 1 e 64 caracteres |
| `health` | Estado observado na criação do heartbeat |
| `counters` | Totais desde o início da sessão |

Campos de `health`:

- `radio_initialized`: último estado conhecido da inicialização/recepção, não garantia de integridade de todas as transações SPI.
- `wifi_connected`: conexão Wi-Fi estabelecida.
- `ntp_synchronized`: relógio considerado sincronizado pelo firmware.
- `queue_size` e `queue_capacity`: ocupação e limite; tamanho não pode superar a capacidade.
- `free_heap_bytes`: heap livre no C3; registrar para avaliar TLS junto com LoRa.
- `wifi_rssi_dbm`: RSSI Wi-Fi ou `null`; não confundir com RSSI LoRa.

Contadores obrigatórios em `counters`:

- `lora_valid`: leituras LoRa aceitas pelo decodificador.
- `lora_rejected`: eventos descartados por CRC, tamanho, versão ou valores inválidos.
- `spi_errors`: ocorrências detectadas de falha SPI, conforme pontos de diagnóstico do firmware.
- `http_failed_attempts`: tentativas de envio que não obtiveram confirmação HTTP válida, tanto leituras como heartbeat.
- `readings_acknowledged`: eventos distintos retirados da fila por confirmação `stored` ou `duplicate`; não contar tentativas.
- `queue_dropped`: leituras perdidas por estouro da fila ou descarte operacional permanente.

Os totais reiniciam na nova sessão e não devem diminuir dentro dela. Uma rejeição LoRa pode ser consequência de SPI instável; os contadores não provam causas exclusivas nem medem todos os pacotes perdidos pelo ar.

Resposta **HTTP 200** ecoa versão, gateway, sessão e `heartbeat_seq`, e acrescenta `server_received_at`, conforme o schema. Heartbeats duplicados ou antigos na mesma sessão não devem regredir contadores/estado atual nem atualizar a liveness como se fossem um heartbeat novo. Não reenviar backlog de heartbeat: substituir snapshots antigos pelo estado atual ao reconectar. Armazenar sessões separadamente para que tentativas atrasadas de uma sessão antiga não substituam automaticamente a sessão ativa.

## Horário e dados atrasados

- `gateway_received_at` é o horário de recepção no C3, não o instante de aquisição no S3.
- `ingested_at` é atribuído pelo servidor ao persistir a primeira vez; não enviado pelo C3.
- Sem NTP, usar o atraso monotônico `upload_uptime_ms - gateway_rx_uptime_ms` como metadado, sem inventar horário exato.
- O desktop mostra separadamente idade da leitura e conexão do gateway. Uma fila antiga enviada após reconexão não deve fazer uma medição aparecer como atual.
- A entrega tem retentativas com idempotência; não há promessa de entrega após reset/fila cheia enquanto a fila for em RAM.

## Erros HTTP

| HTTP | `error.code` | Tratamento pelo C3 |
|---|---|---|
| 400 | `INVALID_REQUEST` | JSON malformado; não repetir automaticamente |
| 401 | `UNAUTHORIZED` | Credencial ausente/inválida; aguardar configuração |
| 403 | `FORBIDDEN` | Gateway incompatível com credencial; aguardar configuração |
| 409 | `EVENT_CONFLICT` | Investigar identidade/conteúdo; não sobrescrever |
| 413 | `PAYLOAD_TOO_LARGE` | Corrigir tamanho; reduzir lote sem mudar IDs |
| 422 | `INVALID_REQUEST` ou `UNSUPPORTED_SCHEMA` | Dados/versão inválidos; não repetir o mesmo lote automaticamente |
| 429 | `RATE_LIMITED` | Respeitar `Retry-After` em segundos |
| 503 | `SERVICE_UNAVAILABLE` | Repetir com atraso exponencial limitado e jitter |

Timeout, falha de DNS/TLS e demais respostas 5xx mantêm as leituras pendentes para nova tentativa espaçada. Certificado inválido não deve provocar desativação da validação TLS. O parser deve lidar com respostas sem JSON, como erros de proxy, preservando o lote e o diagnóstico HTTP.

Formato de erro definido em `error-response.schema.json`: `schema_version` e objeto `error` com `code`, `message`, `retryable` e `details` (array vazio se não houver campo específico). `details.path` identifica o campo, sem repetir credenciais ou payloads sensíveis. Erros nunca contêm confirmações.

## Handoff para a API Go

Implementar primeiro os dois endpoints, migração da chave UNIQUE e validações acima. Fixtures em `examples/` servem como testes de integração, incluindo a resposta de duplicata. O schema é uma referência de contrato, mas não substitui autenticação, validação cruzada ou transação no banco.

Casos de aceite:

1. Leitura válida persistida e confirmada após commit.
2. Mesmo evento reenviado: uma medição no banco e `duplicate` na resposta.
3. Conflito de conteúdo: 409 e nenhuma nova inserção do lote.
4. RMS ausente com `UNKNOWN` e temperatura ausente aceitos como NULL.
5. RMS ausente com `NORMAL`/`ANOMALOUS`, número não finito e versão incompatível rejeitados.
6. Gateway diferente da credencial rejeitado.
7. Dois POST concorrentes idênticos não duplicam a medição.
8. Heartbeat antigo não faz contadores regredirem.
9. `event_seq` repetido no lote, timestamp monotônico inconsistente e fila maior que capacidade rejeitados.
10. Dispositivo sem motor cadastrado preservado para associação posterior.

As consultas desktop de histórico/cadastro continuam sendo responsabilidade da API em desenvolvimento. Este contrato fecha apenas a fronteira do firmware C3 com o servidor, para permitir implementação em paralelo.
