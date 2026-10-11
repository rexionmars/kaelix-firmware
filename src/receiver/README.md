# Receptor de bancada — ESP32-C3 Super Mini + RA-02

O ESP32-S3 continua com `esp32-s3-bench`, MPU e rádio transmissor.
Este firmware recebe pacotes v2 de 21 bytes, valida CRC-16, versão,
estado e campos de medição antes de mostrar os dados. Temperatura NaN
é exibida como `NA`; `UNKNOWN` é esperado no transmissor de bancada.
O gateway Python em `experiments/gateway` continua usando v1: não confundir
com este receptor físico v2.

## Formato dos logs e captura do ensaio

Cada mensagem da aplicação traz tempo desde o boot, dispositivo, nível
e evento. Exemplo ilustrativo:

```text
[+00:00:12.345][gateway][INFO][RX] RX id=D6F61B44 seq=1 state=UNKNOWN diag=OK rms=0.003598 g temp=NA rssi=-55.0 dBm snr=13.5 dB crc=OK
[+00:00:15.000][gateway][INFO][HEALTH] health version=0x12 op=0x8D irq=0x00 dio0=0 rssi=-122.0 rx=1 reject=0 poll=0
[+00:00:20.000][gateway][ERROR][SPI] SPI_UNSTABLE irq=0x70 versions=0x00/0x12; retry initialization
```

`INFO` indica estado e leituras, `DEBUG` detalha o diagnóstico,
`WARN` marca rejeições/eventos inesperados e `ERROR` marca falhas.
`RX` identifica pacotes válidos; `IRQ`, eventos do rádio; `HEALTH`,
estado periódico; `SPI`, diagnóstico do barramento; `REJECT`, descartes.

`+HH:MM:SS.mmm` é tempo de execução da placa, reiniciado no boot,
sem sincronização com o S3. Para incluir horário do computador e salvar
o monitor em arquivo, execute (substitua a porta pela atual):

```bash
pio device monitor -e esp32-c3-receiver -b 115200 --port /dev/cu.usbmodem2101 -f time -f log2file
```

O horário acrescentado pelo computador indica quando a linha foi lida,
não necessariamente o instante exato de aquisição no S3.

## Ligações (com alimentação desligada)

| RA-02 | GPIO/terminal do C3 Super Mini |
|---|---|
| VCC | 3V3 regulado |
| Todos os GND | GND |
| SCK | GPIO4 |
| MISO | GPIO5 |
| MOSI | GPIO6 |
| NSS/CS | GPIO7 |
| RESET | GPIO3 |
| DIO0 | GPIO1 |
| DIO1–DIO5 | Sem conexão para este receptor não bloqueante |

Use os nomes GPIO impressos na placa, não a posição física do conector.
Pinos distintos dos straps 2/8/9 e USB 18/19, conforme a
[documentação do C3](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c3/schematic-checklist.html).
O RA-02 requer alimentação estável de 3,3 V e GND comum ao seu C3.
Antena adequada de 433 MHz em ambos os módulos. Não conectar os dois
conjuntos por fios; cada rádio conecta somente ao seu microcontrolador.

## Gravar

O perfil pressupõe C3 Super Mini com flash de 4 MB, USB Serial/JTAG nativo.
É independente da flash/PSRAM do S3. A pinagem SPI é explícita.
Para evitar seleção da placa errada, deixe apenas o C3 conectado ao USB
na gravação inicial (feche os monitores seriais antes):

```bash
pio run -e esp32-c3-receiver -t upload
pio device monitor -e esp32-c3-receiver -b 115200
```

Se não entrar em gravação automaticamente: segure BOOT, pressione e solte
RESET, solte BOOT e tente o upload. Após gravar, pressione RESET normalmente.
Se a porta mudar, execute `pio device list` e selecione a porta do C3 com
`--upload-port` no upload ou `--port` no monitor.

Com as duas placas conectadas, especifique suas portas separadamente.
O S3 precisa estar executando `esp32-s3-bench`, não `lora-probe`.
Depois de abrir o monitor do C3, alimente o S3 para iniciar as transmissões.

## Saída esperada (exemplo, valores ilustrativos)

```text
[gateway] rf_init=0 version=0x12
[gateway] listening 433MHz SF9 BW125 CR4/5 sync=0x12
[gateway] RX id=12345678 seq=9 state=UNKNOWN diag=OK rms=0.005984 g temp=NA rssi=-45.0 dBm snr=9.0 dB crc=OK
```

Frequência 433 MHz, SF9, BW125 kHz, CR4/5, sync 0x12 e preâmbulo 8 são
compartilhados em `src/comms/lora_config.h` com o transmissor. O rádio usa
SPI a 100 kHz. O callback DIO0 apenas sinaliza a chegada: leitura,
validação e log ocorrem no loop. A recepção é rearmada após cada quadro.

`rf_init` diferente de zero informa o retorno do RadioLib; a inicialização
é tentada novamente a cada 5 s. `read error=-7` indica erro do CRC do rádio.
`reject decode` usa: 1 comprimento, 2 CRC da aplicação, 3 versão,
4 estado, 5 medição inválida. Quadros rejeitados não viram leituras.

A sequência é exibida para inspeção; não existe persistência, painel,
contagem automática de perdas nem confirmação enviada ao transmissor.
Primeiro confirme vários pacotes no monitor, depois faça ensaios de distância.

Teste do decodificador no computador:

```bash
pio test -e native -f test_packet_decode
```

## Diagnóstico: `rf_init=-2 version=0x00`

`-2` é `RADIOLIB_ERR_CHIP_NOT_FOUND`. A leitura de RegVersion deveria
retornar `0x12`; `0x00` não identifica o SX1278. Isso ocorre antes da
recepção e não depende de o transmissor estar ligado.

Para separar a biblioteca do acesso SPI, conecte somente o C3 ao USB,
feche o monitor e grave o diagnóstico isolado:

```bash
pio run -e lora-probe-c3 -t upload
pio device monitor -e lora-probe-c3 -b 115200
```

Pressione RESET depois de abrir o monitor. O teste usa os mesmos pinos
do receptor, pulsa RESET do rádio e lê RegVersion por GPIO e SPI de hardware,
sem RadioLib. O esperado é `0x12` em todas as leituras.

- GPIO e hardware estáveis em `0x12`: investigar a inicialização do receptor.
- Ambos em `0x00`, `0xFF` ou valores variáveis: conferir alimentação, GND,
  RESET, CS e contatos das linhas SPI; o resultado sozinho não prova defeito
  no módulo.
- GPIO estável e hardware falhando: investigar configuração e sinais do SPI
  de hardware.

Com a alimentação desligada, confira GPIO4=SCK, GPIO5=MISO, GPIO6=MOSI,
GPIO7=NSS, GPIO3=RESET e GPIO1=DIO0. VCC deve ir a 3V3 e todos os GND
do módulo devem ter conexão ao GND do C3. Se houver multímetro, confira
a tensão diretamente entre VCC e GND do RA-02 durante a inicialização.

Depois do diagnóstico, regrave `esp32-c3-receiver` para receber os pacotes.

## Diagnóstico: rádio inicializa, mas não aparecem pacotes

O receptor consulta RxDone por SPI a cada 10 ms além de observar DIO0.
Quando encontra um pacote completo, imprime `rx_event via=DIO0` ou
`rx_event via=SPI`. O segundo caso significa que o callback não havia
sinalizado o evento no momento da consulta; não prova sozinho uma falha
elétrica em DIO0. A consulta permite receber mesmo sem o callback.

Na inicialização, `config` exibe os registradores reais de frequência,
modem e sync. Para 433 MHz, BW125, CR4/5, cabeçalho explícito, SF9 e CRC
ativo, espera-se `frf=6C4000 modem1=0x72 modem2=0x94 sync=0x12`.

A cada 5 s, `health` mostra versão, modo, IRQ, nível DIO0, RSSI instantâneo
do canal e contadores de pacotes válidos (`rx`), rejeitados (`reject`) e
eventos atendidos sem callback (`poll`). O RSSI instantâneo não identifica
o transmissor nem confirma que um pacote chegou.

- `version=0x12 op=0x8D`: SX1278 identificado e modo LoRa RX contínuo
  com o bit LowFrequencyModeOn ativo para 433 MHz.
- Bit `0x40` de `irq`: recepção concluída (RxDone).
- Bit `0x10`: cabeçalho válido observado; não confirma o payload completo.
- Bit `0x20`: erro de CRC do payload.
- `irq=0x00 rx=0`: nenhum evento de recepção registrado nessa consulta;
  não basta para distinguir transmissor parado, parâmetros incompatíveis
  ou problema no enlace de rádio.
- `SPI_VERSION_LOST`: versão deixou de ser `0x12`; a inicialização será
  tentada novamente após 5 s.
- `SPI_UNSTABLE`: antes de processar RxDone, pelo menos uma das duas
  leituras consecutivas do identificador falhou. Esse evento não é contado
  como pacote recebido; a inicialização será tentada novamente após 5 s.
- `after_reject` e `after_read_error`: registradores observados após uma
  rejeição, para verificar se o rádio continua respondendo. Leituras corretas
  nesses pontos não excluem falhas SPI em outras transações.

Mantenha o S3 com `esp32-s3-bench` ligado simultaneamente e capture pelo
menos 30 s do monitor do C3. Se não houver RxDone, confira também o log
do S3 alimentado pela fonte para confirmar que continua completando TX.
