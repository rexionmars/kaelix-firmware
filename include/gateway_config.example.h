#pragma once
// Copiar para gateway_config.local.h (ignorado pelo Git).
#define KAELIX_WIFI_SSID ""
#define KAELIX_WIFI_PASSWORD ""
#define KAELIX_API_BASE_URL "" // HTTPS, sem / final e sem /api/v1
#define KAELIX_GATEWAY_ID "" // UUID cadastrado na API
#define KAELIX_GATEWAY_TOKEN ""
#define KAELIX_TLS_CA_PEM "" // CA PEM; pode usar literal R"PEM(...)PEM"
// HTTP só para teste em LAN. Nunca desabilitar validação de certificado HTTPS.
#define KAELIX_ALLOW_HTTP_LAN 0
