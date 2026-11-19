# NASCERR Image Upload Tool

Ferramenta de linha de comando para envio de imagens de firmware para o satélite NASCERR via UART, utilizando o protocolo de frames com validação CRC16 e recuperação automática de erros.

---

## Dependências

- Python 3.10 ou superior
- [pyserial](https://pypi.org/project/pyserial/)

```bash
pip install pyserial
```

---

## Uso rápido

```bash
# Upload normal
python nascerr_img_upload.py --port COM3 --file image.bin

# Monitorar a serial sem enviar nada
python nascerr_img_upload.py --port COM3 --monitor

# Upload passo a passo com confirmação manual em cada fase
python nascerr_img_upload.py --port COM3 --file image.bin --step

# Abortar transmissão em curso e monitorar
python nascerr_img_upload.py --port COM3 --abort --monitor
```

---

## Parâmetros

| Parâmetro | Padrão | Descrição |
|---|---|---|
| `--port` | obrigatório | Porta serial (`COM3`, `/dev/ttyUSB0`, etc.) |
| `--baud` | `2000000` | Baudrate |
| `--file` | — | Caminho para o arquivo `.bin` da imagem |
| `--chunk` | `2112` | Tamanho de cada chunk em bytes (máx. 2112) |
| `--retry` | `1000` | Número máximo de tentativas por chunk |
| `--abort` | — | Envia `IMG_ABORT` antes de qualquer outra ação |
| `--monitor` | — | Após o upload (ou sem `--file`), fica lendo a serial |
| `--step` | — | Modo depuração: confirma cada fase manualmente |
| `--verbose` | — | Exibe bytes de cada frame enviado |
| `--test` | — | Executa self-test local sem hardware |

---

## Protocolo de frames

Todos os frames seguem o formato:

```
[ MAGIC:2 | CMD:1 | LEN:2 | PAYLOAD:N | CRC16:2 ]
```

- Todos os campos multi-byte em **big-endian**
- `MAGIC` = `0xA5C3`
- `CRC16` calculado sobre `MAGIC + CMD + LEN + PAYLOAD` com polinômio `0xA001`, init `0xFFFF`

### Comandos

| CMD | Byte | Payload | Descrição |
|---|---|---|---|
| `IMG_START` | `0x01` | `total_size(4B) + crc16(2B)` | Inicia upload: declara tamanho total e CRC16 esperado |
| `IMG_CHUNK` | `0x02` | `chunk_id(4B) + data(N B)` | Envia bloco de dados identificado por ID sequencial |
| `IMG_FINISH` | `0x03` | — | Solicita validação final da imagem na flash |
| `IMG_ABORT` | `0x04` | — | Cancela e limpa a área de imagem |

### Respostas do firmware

| Byte(s) | Significado |
|---|---|
| `0x06` | ACK — operação aceita |
| `0x15 + id_hi + id_lo` | NACK estendido — firmware informa o `next_expected_id` (2 bytes big-endian) |

---

## Fluxo de upload

```
PC                              STM32
│                                 │
│──── IMG_START ────────────────► │  declara total_size + crc16_esperado
│ ◄─── ACK ──────────────────────│  firmware apaga área de flash
│                                 │
│──── IMG_CHUNK (id=0) ─────────► │  escreve na flash em bytes_done
│ ◄─── ACK ──────────────────────│  next_expected_id++
│                                 │
│──── IMG_CHUNK (id=1) ─────────► │
│ ◄─── ACK ──────────────────────│
│          ...                    │
│──── IMG_CHUNK (id=63) ────────► │
│ ◄─── ACK ──────────────────────│
│                                 │
│──── IMG_FINISH ───────────────► │  relê todos os bytes da flash
│                                 │  recalcula CRC16
│ ◄─── ACK ──────────────────────│  valid = 0xAA gravado no header
```

---

## Recuperação de erros

### Retransmissão por CRC de frame

Cada chunk tem CRC16 próprio validado pelo firmware antes de gravar. Se o frame chegar corrompido (ruído UART, ORE), o firmware envia NACK com `next_expected_id = chunk atual`. O Python retenta o mesmo chunk automaticamente.

### Recuperação de fora-de-ordem

Quando frames duplicados no ring buffer DMA fazem o Python avançar erroneamente, o firmware detecta o `chunk_id > next_expected_id` e responde com NACK estendido informando o ID correto. O Python então:

1. Reseta o estado `confirmed[]` a partir do ID indicado
2. Volta exatamente ao chunk que o firmware espera
3. Retransmite em sequência

```
Python  id=5 ──► FW espera id=4 → NACK (next_exp=4)
Python volta para id=4, reseta confirmed[4..63]
Python  id=4 ──► FW aceita → ACK
Python  id=5 ──► FW aceita → ACK
...
```

### Idempotência na flash

O firmware controla o endereço de gravação internamente via `bytes_done`. Se o Python reenviar um chunk com `chunk_id < next_expected_id` (duplicado confirmado), o firmware responde ACK sem re-gravar, evitando double-write destrutivo em NAND flash.

---

## Validação final (FINISH)

O comando `IMG_FINISH` instrui o firmware a:

1. Ler de volta **todos** os bytes gravados na W25N01G, página a página
2. Recalcular o CRC16 acumulado sobre os dados lidos da flash
3. Comparar com o `crc16_expected` recebido no `IMG_START`

Esta etapa detecta falhas silenciosas de escrita na NAND que os CRCs de frame individuais não cobrem. Só após validação bem-sucedida o header é marcado com `valid = 0xAA`.

> O timeout do FINISH no Python é maior que o dos chunks (60 s) para acomodar o tempo de leitura de ~135 KB de flash via SPI.

---

## Hardware testado

| Componente | Detalhe |
|---|---|
| MCU | STM32 (FreeRTOS + HAL) |
| UART | UART4 @ 2 Mbaud, DMA circular (ring buffer 8192 B) |
| Flash | W25N01G (NAND, 1 Gbit) |
| Conversor USB-serial | FTDI de alta qualidade + cabo blindado |

### Observações sobre ruído UART

A recepção usa DMA circular com detecção de IDLE line (`HAL_UARTEx_ReceiveToIdle_DMA`). A interrupção de erro (`EIE`) está desabilitada para evitar que Noise Errors (NE) abortem o DMA e causem cascata de Overrun Errors (ORE). A integridade dos dados é garantida exclusivamente pelo CRC16 de frame — bytes corrompidos por ruído resultam em falha de CRC e retransmissão automática.

---

## Self-test

Verifica a integridade do código de montagem de frames sem hardware:

```bash
python nascerr_img_upload.py --port dummy --test
```

Testa: montagem de START/CHUNK/FINISH/ABORT, encode big-endian do `chunk_id`, CRC16 de todos os tipos de frame, e decode do NACK estendido.