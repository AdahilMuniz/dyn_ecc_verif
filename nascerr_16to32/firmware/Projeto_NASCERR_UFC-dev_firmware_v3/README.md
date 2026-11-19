# NASCERR — Nanosatellite Error Resilient Radiation Mission

> **Status do software:** *em desenvolvimento*. Nesta fase, **a única interface suportada** é a **CLI** `nascerr_tool.py` (Python) para enviar e receber comandos via **I²C**.
> A especificação de protocolo, registradores e formatos de dados segue o **ICD NASCERR (11.09.2025 – RV01)**.

---

## Sobre o Projeto

O **NASCERR** (*Nanosatellite Error Resilient Radiation Mission*) é um experimento do **LESC/UFC** que valida o **ROBC (Robust On-Board Computer)** em ambiente LEO, com mitigação de radiação por **ECC**. O payload usa um **STM32** como *slave* I²C e expõe um **protocolo de registradores** com **CRC-16** para garantir integridade fim-a-fim. O item básico de telemetria do experimento tem **88 bytes** (cabeçalho + **ECC telemetry** de 30 B + **MDCA** de 34 B). &#x20;

---

## Arquitetura (visão atual)

* **Master (Host)**: PC/Raspberry executando Python; envia telecomandos, lê telemetria e aciona atualização de firmware.
* **Slave (NASCERR)**: dispositivo I²C **endereço 0x55**, frequência típica 100 kHz; leitura/escrita mapeada por registradores com **CRC-16 (polinômio 0xA001)**. &#x20;

---

## CLI oficial (fase atual)

A CLI **`nascerr_tool.py`** concentra as operações disponíveis:

```text
i2cscan           Varre o barramento I²C (usa i2cdetect -y 1)
status            Mostra o status geral (STAT)
boot-status       Mostra informações de bootloader/versão (BOOT_STAT)
expstatus         Mostra o status de experimentos (EXP_STAT)
expsum-idx        Define o offset de página do sumário (EXP_SUM_IDX)
expsum            Lê o sumário (EXP_SUM_DATA)
send              Envia um único NASCERR_CMD (WRITE_CMD ou READ_CMD)
send-preset       Envia N comandos de teste sequenciais
read-idx          Lê resultados por página/qtd_blocos (READ_CMD)
read-exp          Lê TXB pronto (get_experiment)
update-fw         Atualiza firmware (BOOT_DATA/BOOT_STAT)
clear-db          Apaga regiões da flash (LOGS, SCHEDULER, EXPERIMENT, BACKUP)
gps-data          Grava bytes de GPS no registrador GPS
```

### Pré-requisitos

* **Python 3.10+** e *venv*
* Dependências Python do projeto (`requirements.txt`)
* **i2c-tools** (para `i2cscan`, em Linux/Debian/Raspberry Pi)

### Instalação rápida

```bash
git clone <repo-NASCERR>
cd Software_NASCERR/NASCERR_Application
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

> Em Raspberry/PC com I²C habilitado, confirme o dispositivo com `i2cdetect -y 1`.
> Caso necessário habilite o I2C em `raspi-config`
---

## Uso essencial

### 1) Escanear I²C

```bash
python nascerr_tool.py i2cscan
```

### 2) Ler status e sumários

```bash
python nascerr_tool.py status
python nascerr_tool.py expstatus
python nascerr_tool.py expsum-idx --offset 60
python nascerr_tool.py expsum
```

* `EXP_SUM_IDX` seleciona a página (0–191) dos **itens não lidos**; `EXP_SUM_DATA` retorna até **30 entradas** com contador total e *offset* da primeira entrada da página. **Tamanho** do bloco: **216 bytes** (dados+CRC). &#x20;

### 3) Solicitar telemetria de resultados (READ\_CMD)

```bash
python nascerr_tool.py read-idx --page 10 --qtd_block 3
python nascerr_tool.py read-exp
```

**Fluxo padrão**:

1. Envie **READ\_CMD** informando `block` (página) e `qtd_block` (quantos itens preparar).
2. Aguarde o **STAT** indicar pronto;
3. Leia **IDX** para saber **exactamente quantos bytes** estão prontos em **TXB** (payload + CRC16);
4. Leia **TXB** e **divida o payload em blocos de 88 B** (`NASCERR_EXPERIMENT`).&#x20;

### 4) Enviar comando individual (WRITE/READ)

```bash
python nascerr_tool.py send --cmd_id 7 --block 7 --qtd 1 --write
```

### 5) Atualização de firmware (OTA)

```bash
python nascerr_tool.py update-fw --file build/app.bin --version 0x00010008
```

O processo usa **BOOT\_DATA** (escrita) e **BOOT\_STAT** (leitura/monitoramento) com comandos **DOWNLOAD\_INIT (0x01)** → **DATA\_TRANSFER (0xAA)** → **DOWNLOAD\_FINISH (0x02)**. **Monitore BOOT\_STAT continuamente** entre etapas (inclusive se `ERROR_DETECTED` aparecer brevemente) e avance apenas quando `IN_DOWNLOAD` estiver setada. Após `DOWNLOAD_FINISH`, o sistema reinicia e o bootloader valida o **SHA-256**.  &#x20;

> **Dica:** durante **DATA\_TRANSFER**, confirme a recepção verificando em `BOOT_STAT` o `last_packet_id`, `DOWNLOAD_SUCCESS` e ausência de `ERROR_DETECTED`.&#x20;

---

## Registradores principais (resumo)

| Nome           | End. | R/W | Tamanho (bytes) | Uso resumido                                                             |
| -------------- | ---- | --- | --------------- | ------------------------------------------------------------------------ |
| `STAT`         | 0x01 | R   | 7               | Estado operacional; indica quando `TXB` está pronto.                     |
| `IDX`          | 0x02 | R   | 4               | Tamanho pronto para leitura em `TXB` (payload+CRC).                      |
| `TXB`          | 0x03 | R   | 2066            | Buffer de telemetria do experimento (lido após `IDX`).                   |
| `CMD`          | 0x04 | W   | 31              | Telecomando de experimento (inclui READ/WRITE e campos de agendamento).  |
| `EXP_STAT`     | 0x13 | R   | 18              | Sumário geral (contadores/flags de experimentos).                          |
| `EXP_SUM_IDX`  | 0x14 | W   | 3               | Seleciona a página de sumário (0–191).                                   |
| `EXP_SUM_DATA` | 0x15 | R   | 216             | Página com até 30 **DBItems** (id, num\_block, status).                  |
| `BOOT_DATA`    | 0x10 | W   | 263             | Recepção dos pacotes de atualização (INIT / TRANSFER / FINISH).          |
| `BOOT_STAT`    | 0x11 | R   | 15              | Estado do boot/OTA (flags de processo e versão).                         |

> **Integridade:** todas as mensagens incluem **CRC-16 (0xA001)** anexado ao final. O receptor recalcula e compara; divergência implica falha. &#x20;

---

## Formatos de dados importantes

* **Item de telemetria (`NASCERR_EXPERIMENT`)**:
  `id (U32), block (U16), data_length (U32), time_delta (U32), number_errors (U32), corrected_data (U32), percent_error (F32), ecc_telemetry (30 B), mdca_data (34 B)` → **88 B**.&#x20;
* **ECC telemetry (30 B)**: inclui contadores de erros (1-bit, 2-bits, ≥3-bits), erros por regiões, `ecc` aplicado e `ecc_status`. &#x20;

---

## Exemplos rápidos

* **Ler 3 itens da página 10 e obter TXB:**

  ```bash
  python nascerr_tool.py read-idx --page 10 --qtd_block 3
  python nascerr_tool.py read-exp
  ```

  > O `IDX` trará `exp_len = (N × 88) + 2` (payload + CRC), para você saber quantos bytes ler de `TXB`.&#x20;

* **Atualizar firmware (versão 0x00010008):**

  ```bash
  python nascerr_tool.py update-fw --file build/app.bin --version 0x00010008
  ```

  > Siga o fluxo `DOWNLOAD_INIT` → `DATA_TRANSFER` → `DOWNLOAD_FINISH` e monitore `BOOT_STAT` a cada etapa. &#x20;

---

## Configuração do Sistema (NASCERR_Config.h)

O comportamento do firmware é ajustado via `NASCERR_Config.h`. Abaixo, os *flags* e seus **valores padrão atuais**:

```c
/* Debug and simulation options */
#define MOCK_COMMAND        0   /* Mock de comandos no Slave (sem origem externa)    */
#define MOCK_EXPERIMENT     0   /* Mock de experimento (gera dados fictícios)        */
#define ENABLE_LOGS         1   /* Habilita sistema de logs via UART (115.200 bps)   */
#define SOFTWARE_DEBUG      0   /* Modo debug de software (ex.: desabilita IWDG)     */
```

---

### Caminhos relativos (a partir do repositório raiz)
- Firmware (aplicação): `Firmware_CubeSat/dev_nascerr_firmware/`
- Bootloader: `Firmware_CubeSat/dev_nascerr_bootloader/`
- Software (CLI e app): `Software_NASCERR/NASCERR_Application/``

---

## ⚠️ Aviso sobre o *linker script* da aplicação (debug vs. bootloader)

Ao **depurar a aplicação diretamente** pela IDE (sem passar pelo bootloader), é necessário **ajustar a região `FLASH` no `.ld`** para que a aplicação seja posicionada no **início da Flash** e o vetor de interrupções fique em `0x08000000`.

No`.ld` existem duas opções típicas — **use apenas UMA por vez**:

```ld
/* --- Marcações da aplicação  --- */
_APP_VERSION = 0x00010800;   /* versão da aplicação, usada na assinatura   */
_APP_MAGIC   = 0x12345678;   /* número mágico (assinatura/metadata)        */

/* --- Definição de memórias --- */
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 640K
  RAM2  (xrw) : ORIGIN = 0x10000000, LENGTH =  64K
  RAM3  (xrw) : ORIGIN = 0x200A0000, LENGTH = 384K

  /* [A] DEBUG DIRETO (sem bootloader): aplicação em 0x08000000 */
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 894K

  /* [B] PRODUÇÃO (com bootloader): aplicação deslocada (exemplo)
     Habilite esta linha e comente a anterior quando usar bootloader.
     Ajuste o endereço conforme o espaço reservado ao bootloader/metadata. */
  /* FLASH (rx)  : ORIGIN = 0x08020000, LENGTH = 894K */
}
```
---

## Licença

Projeto acadêmico do **LESC – UFC**. Direitos e diretrizes seguem as políticas institucionais de pesquisa e inovação.

---
