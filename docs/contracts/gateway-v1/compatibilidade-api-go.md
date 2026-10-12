# Compatibilidade com a API Go implementada

Revisão atualizada em 11/10/2026 dos arquivos em `/Users/fox/estudos/lauan/kaelix-Ai-center/`: `apps/server/README.md`, `packages/contracts/contracts.go` e `apps/server/internal/api/validation.go`.

A API foi atualizada para adotar o contrato JSON v1 deste diretório. Os cinco schemas e todos os exemplos em `packages/contracts/gateway-v1/` são idênticos aos nossos. A revisão anterior, que apontava divergências, foi substituída por este registro.

## Correções confirmadas

| Ponto | Situação atual |
|---|---|
| Estado de anomalia | A validação aceita `NORMAL`, `ANOMALOUS`, `UNKNOWN`. |
| Sequência do dispositivo | `Reading.DeviceSequence` usa `*uint32`, compatível com o pacote LoRa. |
| RMS ausente | `null` só é permitido com `UNKNOWN`; RMS negativo é rejeitado. |
| Confirmação de leituras | Usa `schema_version`, IDs, `server_received_at` e `acknowledged` com `stored` ou `duplicate`. |
| Heartbeat | Usa `heartbeat_seq`, `health`, `counters` e resposta com identidade e horário do servidor. |
| Erros | Incluem versão, código, mensagem, `retryable` e `details`. |
| Identificadores | Gateway e sessão exigem UUID em minúsculas; sensor mantém oito dígitos hexadecimais em maiúsculas. |
| Limites | Lotes de 1 a 8 leituras, sequência uint32 e uptime/contadores dentro do intervalo inteiro seguro do TypeScript. |

O README também documenta confirmação após commit, duplicatas, conflito atômico, sessões e deduplicação de heartbeat. O C3 deve remover da fila somente eventos confirmados e enviar apenas o heartbeat atual após reconectar.

## Verificação realizada

- Comparação dos schemas e exemplos entre os dois repositórios: nenhuma diferença.
- Inspeção dos tipos Go, da validação e da documentação: os três problemas anteriores foram corrigidos.
- `go test ./...` em `apps/server`: passou. Testes de integração PostgreSQL são ignorados sem `TEST_DATABASE_URL`; esta execução não valida banco nem comunicação física.

O README oferece `./scripts/test-server-postgres.sh` para a validação com PostgreSQL real. Esta revisão não comprova o percurso C3 → HTTPS → banco → desktop.

## Próximos passos no C3

O contrato está alinhado. O transporte foi implementado no firmware C3, com as funções abaixo; falta configurar e validar no hardware:

1. Wi-Fi e HTTPS com validação do certificado.
2. Identidade do gateway, sessão de boot e credencial Bearer.
3. Conversão dos pacotes LoRa válidos para JSON v1, preservando `UNKNOWN` e temperatura `null`.
4. Fila de leituras, envio em lotes e reenvio com a mesma identidade e conteúdo de evento.
5. Validação das confirmações antes de retirar eventos da fila.
6. Heartbeat atual com saúde e contadores.

Configuração, política de fila e roteiro de bancada estão em [INTERNET.md](../../../src/receiver/INTERNET.md).

O desktop ainda precisa consumir a API; o README do servidor registra essa integração como pendente.

## Acesso para teste físico

O Docker Compose publica API e banco apenas em loopback. O C3 em Wi-Fi não alcança `127.0.0.1` do computador: nele esse endereço é o próprio C3. Para teste pela LAN, preparar um listener/proxy acessível na rede e usar o endereço do computador; HTTP deve ficar restrito ao desenvolvimento. Para o percurso pela internet, usar endpoint público HTTPS e certificado validado pelo C3.

Cadastrar um gateway na API gera sua credencial. O firmware precisará de ID, token, endpoint, Wi-Fi e configuração de confiança TLS; credenciais ficam fora do repositório e dos logs.
