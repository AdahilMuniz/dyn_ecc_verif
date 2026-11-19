# Experimento de Imagem NASCERR — Plano de Implementação

**Projeto:** NASCERR CubeSat — Caracterização de SEU via Processamento de Imagem  
**Branch:** `dev_image_experiment`  
**Data:** 2026-04-29  
**Status:** Planejamento

---

## 1. Resumo Executivo

O experimento de imagem grava uma imagem de teste conhecida, lida da flash interna do MCU, nos dois chips SRAM (por meio do subsistema de ECC da FPGA), lê os dados de volta após um atraso configurável ou exposição à radiação, e avalia a degradação por meio de métricas padrão de qualidade de imagem (PSNR, SSIM) e de confiabilidade de memória (BER, contagem de erros corrigidos/não corrigíveis). Cada execução produz um resultado em par: um sob o modo ECC selecionado e outro sob Dual Bypass (sem ECC), de forma que o benefício da correção de erros seja diretamente mensurável. A metodologia é idêntica para validação local com o sabotador e para o laboratório de radiação SEU — apenas a fonte de erros muda.

---

## 2. Contexto do Sistema

### Topologia de hardware

```
PC / Raspberry Pi
    │  USB (FT2232H Dual VCP)
    │  Canal A → MCU UART4 (PC10/PA1, 2 Mbps)
    │  Canal B → FPGA UART
    ↓
STM32L4S5 (MCU)
    ├── Flash Interna  ← imagens-fonte (arrays const, barbara_16/48/96/192)
    ├── Flash NAND Externa (W25N01G via SPI3)  ← metadados de resultados
    ├── Escravo I2C  ← interface de comando principal (Python smbus2 → /dev/i2c-X)
    └── FMC → FPGA (M2S025)
                    ↓ Codificação/decodificação ECC em cada palavra SRAM
              SRAM_LOW (U201, IS61WV204816BLL, 2M×16 bits)
              SRAM_HIGH (U200, IS61WV204816BLL, 2M×16 bits)
              Combinadas: barramento de dados 32 bits, 2 MB endereçáveis
```

### Modos ECC (implementados na FPGA)

| Modo | SRAM_LOW | SRAM_HIGH | Descrição |
|---|---|---|---|
| `BYPASS_1` | dados | — | Sem ECC, apenas memória 1 |
| `BYPASS_2` | — | dados | Sem ECC, apenas memória 2 |
| `DUAL_BYPASS` | dados | dados | Ambas as memórias, sem ECC |
| `TBEC` | dados | síndrome | Triple-Bit Error Correction |
| `MRSC` | dados | síndrome | Multi-Row Single-Column |

Para este experimento: **qualquer modo ECC** é emparelhado com **DUAL_BYPASS** na mesma execução, de forma que os resultados sejam diretamente comparáveis.

### Módulos de firmware existentes utilizados

- `NASCERR_ImageProcessing.h/.c` — serialização de imagem, codec RLE, PSNR, SSIM, escrita/leitura SRAM
- `NASCERR_Memory_32bits.h/.c` — `nascerr_memory_write_image_buffer()`, leitura/escrita de bloco
- `NASCERR_Scheduler.h/.c` — orquestração de tarefas, fila de comandos
- `NASCERR_Flash.h/.c` + `flash_w25n01g.h/.c` — persistência de metadados de resultados
- `NASCERR_DB.h/.c` — cabeçalho de banco de dados, gerenciamento de páginas de experimento
- `uart_handler.h/.c` — camada de pacote UART (disponível, ainda não integrada ao loop de comandos)
- `nascerr_protocol.py` + `nascerr_command.py` — camada de comando/resposta Python

---

## 3. Projeto do Experimento

### 3.1 Dados-fonte

As imagens de teste existem como arrays `const uint8_t` na flash interna do MCU (`NASCERR_ImageProcessing.c`):

| Imagem | Tamanho | Pixels | Bytes (palavras 16 bits) |
|---|---|---|---|
| `barbara_16` | 16×16 | 256 | 512 B |
| `barbara_48` | 48×48 | 2.304 | 4.608 B |
| `barbara_96` | 96×96 | 9.216 | 18.432 B |
| `barbara_192` | 192×192 | 36.864 | 73.728 B |

Os arrays `barbara_96` e `barbara_192` precisam ser adicionados (placeholders já existem no cabeçalho).

O MCU lê essas imagens diretamente de sua própria flash por meio do ponteiro const — nenhuma leitura de flash externa é necessária para a imagem-fonte. A flash NAND externa é usada exclusivamente para armazenar resultados e metadados do experimento.

### 3.2 Pré-processamento (lado da escrita)

Duas opções, selecionáveis por comando:

| Modo | Descrição | Consideração |
|---|---|---|
| `IMG_LOSSLESS` | Sem compressão, pixels brutos | Comparação byte a byte exata na leitura |
| `IMG_LOSS_FF` | Compressão RLE | Menor ocupação de SRAM por tile; expõe robustez do RLE ao SEU |

Para caracterização SEU, **`IMG_LOSSLESS` é o modo primário** — a comparação direta de pixels é inequívoca. O RLE pode ser executado como experimento secundário para observar se erros de bit no fluxo comprimido causam degradação em cascata durante a descompressão.

### 3.3 Estratégia de preenchimento da SRAM

A SRAM possui 2 MB (1.048.576 × palavras de 16 bits), dividida em 64 blocos de 32.768 palavras cada.

A imagem é replicada (tiled) pela SRAM até que um número configurado de blocos seja preenchido:

```
para bloco_idx de 0 até N_blocos:
    offset = bloco_idx × TAMANHO_BLOCO_PALAVRAS
    replicar imagem repetidamente até preencher o bloco
    nascerr_memory_write_image_buffer(modo_ecc, offset, buffer, TAMANHO_BLOCO_PALAVRAS)
```

Isso significa que uma imagem 48×48 (2.304 pixels) é replicada ≈14 vezes para preencher um bloco. Para cobertura total da SRAM (64 blocos), a imagem é escrita ≈896 vezes no total, maximizando o número de células de memória expostas à radiação.

**Parâmetros configuráveis:**
- `n_blocks` (1–64): quantos blocos SRAM preencher
- `ecc_mode`: modo ECC para esta metade do experimento
- `compression`: RLE ou sem compressão

### 3.4 Estratégia de execução com ECC

Cada comando de experimento aciona **duas execuções sequenciais na SRAM**:

```
Execução A: ECC selecionado (TBEC / MRSC / BYPASS_1 / BYPASS_2)
    → gravar imagem em N_blocos com modo ECC
    → aguardar (delay_ms ou gatilho externo para exposição à radiação)
    → ler de volta, comparar, coletar métricas
    → armazenar resultado_A

Execução B: DUAL_BYPASS (sem ECC, referência)
    → gravar imagem em N_blocos
    → aguardar (mesmo delay_ms)
    → ler de volta, comparar, coletar métricas
    → armazenar resultado_B
```

A Execução B fornece a linha de base desprotegida. O delta entre A e B quantifica a eficácia do ECC nas mesmas condições.

Se o comando especificar `ecc_mode = DUAL_BYPASS`, as Execuções A e B são idênticas (ambas sem ECC). Isso é útil para calibração de linha de base.

### 3.5 Leitura de retorno e pós-processamento

Após a janela de atraso/exposição:

1. Ler de volta todos os blocos gravados da SRAM (`nascerr_memory_read_block()`)
2. Se RLE foi usado, descomprimir (`img_rle_decompress()`)
3. Reconstruir imagem 2D a partir do vetor linear de blocos (`img_reconstruct_from_blocks()`)
4. Comparar pixel a pixel com a imagem-fonte original

**Métricas calculadas por execução:**

| Métrica | Descrição | Relevância |
|---|---|---|
| `n_errors` | Total de pixels que diferem do original | Contagem primária de SEU |
| `error_rate` | `n_errors / total_pixels` | Taxa normalizada |
| `n_bits_flipped` | Distância de Hamming no nível de bit | Numerador do BER |
| `BER` | `n_bits_flipped / total_bits` | Bit Error Rate |
| `n_corrected` | Erros corrigidos pela ECC da FPGA | Eficácia da ECC |
| `n_uncorrectable` | Erros que passaram pela ECC | Dano residual |
| `MSE` | Erro Quadrático Médio dos valores de pixel | Magnitude da distorção |
| `PSNR` | Peak Signal-to-Noise Ratio (dB) | Qualidade de imagem |
| `SSIM` | Índice de Similaridade Estrutural (0–1) | Qualidade perceptual |
| `affected_blocks` | Máscara de bits dos blocos SRAM com erros | Distribuição espacial |

Todas as métricas são calculadas no próprio chip. O MCU tem capacidade computacional suficiente para PSNR/SSIM em ponto flutuante para imagens até 192×192.

### 3.6 Estrutura de dados do resultado

Uma nova struct `NASCERR_IMAGE_RESULT` (a ser definida junto com `NASCERR_EXPERIMENT`):

```c
typedef struct {
    id_db        id;               // ID do experimento
    uint32_t     timestamp;        // Tick FreeRTOS ao fim da leitura
    NASCERR_MODE ecc_mode;         // Modo ECC utilizado
    ImgSelect_t  image;            // Qual imagem de teste foi usada
    ImgCompression_t compression;  // Modo de compressão utilizado
    uint8_t      n_blocks;         // Número de blocos SRAM preenchidos
    uint32_t     n_errors;         // Contagem de erros no nível de pixel
    uint32_t     n_bits_flipped;   // Contagem de erros no nível de bit
    uint32_t     n_corrected;      // Erros corrigidos pela ECC
    uint32_t     n_uncorrectable;  // Erros que passaram pela ECC
    float        BER;              // Bit Error Rate
    float        PSNR;             // dB
    float        SSIM;             // 0.0–1.0
    float        MSE;
    uint64_t     affected_blocks;  // Máscara de 64 bits dos blocos afetados
    uint16_t     crc;              // CRC16 de todos os campos acima
} NASCERR_IMAGE_RESULT;            // ~64 bytes
```

Dois desses resultados (Execução A + Execução B) são armazenados na flash NAND externa e também carregados no buffer para transmissão via registrador `TXB`.

---

## 4. Implementação do Firmware

### 4.1 Novo tipo de comando

Adicionar ao enum `CommandType_t` em `NASCERR_Scheduler.h`:

```c
CMD_IMAGE_EXPERIMENT      = 6,   // Iniciar experimento de imagem
CMD_IMAGE_RESULT_REQUEST  = 7,   // Solicitar download de página de resultado
```

E uma nova struct de payload de comando (`NASCERR_IMAGE_CMD` já está em `NASCERR_ImageProcessing.h`):

```c
typedef struct {
    id_db            id;          // ID do experimento atribuído pelo host
    NASCERR_MODE     ecc;         // Modo ECC para a Execução A
    nascerr_rpt      repeat;      // Número de ciclos completos do experimento
    ImgSelect_t      image;       // Imagem a utilizar
    ImgCompression_t comp;        // Modo de compressão
    uint8_t          n_blocks;    // Blocos SRAM a preencher (1–64)
    uint32_t         delay_ms;    // Tempo de espera entre escrita e leitura
} NASCERR_IMAGE_CMD;
```

### 4.2 Fluxo de tarefas

A estrutura de tarefas FreeRTOS existente é reutilizada:

```
masterCommTask
    recebe CMD_IMAGE_EXPERIMENT via protocolo I2C
    valida e enfileira em commandQ

nascerrWriteExp (expWriteTask)
    desenfileira CMD_IMAGE_EXPERIMENT
    → chama nascerr_image_run_experiment_write(cmd)
        - carrega imagem do array const
        - compressão RLE opcional
        - preenche N_blocos da SRAM com modo ECC (Execução A)
        - preenche N_blocos da SRAM com DUAL_BYPASS (Execução B)
    → sinaliza FLAG_EXP_READ

nascerrReadExp (expReadTask)
    acionado por FLAG_EXP_READ
    → aguarda delay_ms
    → chama nascerr_image_run_experiment_read(cmd)
        - lê de volta N_blocos (Execução A) → métricas A
        - lê de volta N_blocos (Execução B) → métricas B
        - empacota duas structs NASCERR_IMAGE_RESULT
        - armazena na flash NAND (nascerr_db_addExperiment)
        - carrega buffer TXB com resultado A + resultado B
        - define EXP_STAT = EXPERIMENT_DONE
```

### 4.3 Separação das fases de escrita e leitura

O atraso entre escrita e leitura é a janela de exposição à radiação. No laboratório de radiação, essa janela não é um temporizador de software — ela é acionada externamente (o feixe pode ser ligado/desligado). Portanto, o firmware deve suportar:

- **Modo temporizado**: `delay_ms > 0` → delay do OS, depois leitura automática
- **Modo de gatilho externo**: `delay_ms = 0` → fase de escrita conclui, EXP_STAT sinaliza WRITE_DONE, host envia `CMD_IMAGE_RESULT_REQUEST` separado para iniciar a leitura

O modo de gatilho externo desacopla escrita de leitura, dando ao operador do laboratório de radiação controle sobre a janela de exposição. Ambos os modos usam a mesma lógica de leitura de retorno e métricas.

### 4.4 Transmissão do resultado

Tamanho do resultado: 2 × `NASCERR_IMAGE_RESULT` ≈ 128 bytes → cabe em uma única leitura `TXB` (máximo de 2.066 bytes).

Para dados completos da imagem reconstruída (necessários para visualização):
- Adicionar campo `image_data[]` ao resultado, ou usar um comando separado
- Imagem é dividida em múltiplas leituras TXB (índice de página no payload do comando)
- `barbara_192` = 36.864 bytes → ~18 leituras TXB de 2.048 bytes cada

Fase 1: transmitir apenas a struct de métricas (suficiente para caracterização no laboratório).  
Fase 2: adicionar download completo de dados de imagem para visualização no PC.

---

## 5. Protocolo de Comunicação

### 5.1 Comando (host → STM32)

Usando o wrapper `NASCERR_CTRL` existente sobre o registrador I2C `CMD` (0x04):

```
[2B tamanho][1B tipo_cmd=CMD_IMAGE_EXPERIMENT][payload NASCERR_IMAGE_CMD][2B CRC16]
```

### 5.2 Polling de status (host lê)

Host faz polling do registrador `EXP_STAT` (0x13, 18 bytes) até `req_exp = REQ_EXP_TX_RESULT`.

Estados durante o experimento:
```
WRITE_STARTED → WRITE_DONE → (janela de exposição) → READ_STARTED → READ_DONE → TX_RESULT_READY
```

### 5.3 Recuperação do resultado (host lê TXB)

Host lê registrador `TXB` (0x03). Resposta contém:
```
[NASCERR_IMAGE_RESULT execucao_A][NASCERR_IMAGE_RESULT execucao_B]
```
Host faz ACK com registrador `ACK` (0x0D) como de costume.

### 5.4 Download de dados de imagem (Fase 2)

Novo sub-comando ou `CMD_IMAGE_RESULT_REQUEST` separado com campo `page_idx`. Host itera páginas, cada leitura TXB retorna 2.048 bytes de dados de imagem até que todos os pixels sejam transferidos.

---

## 6. Aplicação Host Python

### 6.1 Controlador do experimento (`nascerr_image_experiment.py`)

```python
class ImageExperiment:
    def send_command(image, ecc_mode, n_blocks, delay_ms, compression)
    def wait_for_write_done()        # poll EXP_STAT
    def trigger_read_back()          # para modo de gatilho externo
    def wait_for_results()           # poll EXP_STAT
    def read_results() -> (ResultA, ResultB)
    def download_image_data(run) -> np.ndarray   # Fase 2
    def run_full_experiment(...)     # wrapper de conveniência
```

### 6.2 Exibição de métricas

Tabela no console após cada execução:

```
┌─────────────────┬──────────────┬──────────────┐
│ Métrica         │ ECC (TBEC)   │ BYPASS       │
├─────────────────┼──────────────┼──────────────┤
│ PSNR (dB)       │ 42,1         │ 28,7         │
│ SSIM            │ 0,9923       │ 0,7841       │
│ BER             │ 1,2e-6       │ 3,8e-5       │
│ Erros           │ 12           │ 389          │
│ Corrigidos      │ 11           │ 0            │
│ Não corrigíveis │ 1            │ 389          │
│ Blocos afetados │ 0x0000000C   │ 0x3FFFFFFFF  │
└─────────────────┴──────────────┴──────────────┘
```

### 6.3 Visualização de imagem (Fase 2)

Após download dos dados de pixel:

```
┌─────────────┬─────────────┬─────────────┐
│   ORIGINAL  │ TBEC result │ BYPASS ref  │
│   (fonte)   │ PSNR=42,1   │ PSNR=28,7   │
│   [imagem]  │  [imagem]   │  [imagem]   │
└─────────────┴─────────────┴─────────────┘
```

Implementado com `matplotlib` ou `PIL`. Salvo como PNG para relatórios de laboratório.

### 6.4 Registro de dados

Cada execução de experimento é salva em um arquivo JSON/CSV:

```json
{
  "timestamp": "2026-05-15T14:32:10",
  "image": "barbara_48",
  "n_blocks": 16,
  "ecc_mode": "TBEC",
  "compression": "sem_compressao",
  "environment": "laboratorio_radiacao",
  "execucao_A": { "PSNR": 42.1, "SSIM": 0.9923, "BER": 1.2e-6, ... },
  "execucao_B": { "PSNR": 28.7, "SSIM": 0.7841, "BER": 3.8e-5, ... }
}
```

---

## 7. Metodologia de Teste

### 7.1 Validação local — modo sabotador

O firmware do STM32 possui um injetor de erros integrado (macro de tempo de compilação `ERROR_ENABLE` em `NASCERR_Memory_32bits.c`). Quando habilitado, ele inverte um bit configurado a cada `ERROR_STRIDE_WORDS` palavras durante escritas na SRAM, simulando SEU.

**Procedimento de validação:**
1. Gravar firmware com `ERROR_ENABLE=1`, `ERROR_BIT_TO_FLIP=1`, `ERROR_STRIDE_WORDS=1024`
2. Executar experimento em `barbara_48`, modo TBEC, 16 blocos, sem compressão
3. Verificar: `n_corrected > 0`, PSNR(TBEC) > PSNR(BYPASS), imagem visualmente íntegra
4. Aumentar densidade de erros (`ERROR_STRIDE_WORDS=128`) até TBEC saturar
5. Verificar que BYPASS mostra mais erros que TBEC na mesma densidade
6. Desabilitar sabotador (`ERROR_ENABLE=0`), verificar execução limpa: `n_errors=0`, `PSNR=∞`

Isso valida o pipeline completo (escrita → ECC → leitura → métricas → transmissão) antes de ir ao laboratório de radiação.

### 7.2 Laboratório de radiação — ambiente SEU real

**Preparação pré-teste:**
- Gravar firmware com `ERROR_ENABLE=0` (sem erros artificiais)
- Calibrar: executar um experimento de linha de base limpa antes de ligar o feixe
- Confirmar `n_errors=0` na linha de base

**Durante a irradiação:**
- Usar modo de gatilho externo (`delay_ms=0`)
- Operador executa fase de escrita → confirma WRITE_DONE no host
- Feixe é ligado para fluência/tempo configurados
- Feixe é desligado → operador aciona leitura de retorno pelo host
- Resultados são registrados automaticamente

**Matriz de experimentos (exemplo):**

| Execução | Imagem | ECC | Blocos | Repetições |
|---|---|---|---|---|
| 1 | barbara_48 | TBEC | 16 | 3 |
| 2 | barbara_48 | MRSC | 16 | 3 |
| 3 | barbara_192 | TBEC | 64 | 1 |
| 4 | barbara_192 | MRSC | 64 | 1 |

Cada execução produz automaticamente a comparação BYPASS emparelhada.

**Repetibilidade:** O campo `repeat` em `NASCERR_IMAGE_CMD` executa o ciclo escrita+leitura+análise N vezes consecutivamente. As estatísticas são calculadas como média entre as repetições para o relatório final.

---

## 8. Roteiro de Implementação

### Fase 1 — Loop central do experimento (firmware)

- [ ] Adicionar arrays const `barbara_96` e `barbara_192` a `NASCERR_ImageProcessing.c`
- [ ] Implementar loop de preenchimento de N blocos SRAM (replicar imagem em N blocos)
- [ ] Implementar loop de leitura de retorno em N blocos
- [ ] Adicionar struct `NASCERR_IMAGE_RESULT`
- [ ] Dividir `nascerr_image_run_experiment()` em fases de escrita e leitura
- [ ] Conectar `CMD_IMAGE_EXPERIMENT` ao caminho de decodificação de comandos do scheduler
- [ ] Implementar lógica de execução dupla (ECC + companheiro BYPASS)
- [ ] Armazenar resultados na flash NAND via camada DB existente
- [ ] Carregar métricas no buffer TXB para recuperação pelo host

### Fase 2 — Protocolo e host (Python)

- [ ] Adicionar serialização de `NASCERR_IMAGE_CMD` a `nascerr_command.py`
- [ ] Adicionar `CMD_IMAGE_EXPERIMENT` ao despacho de comandos em `nascerr_protocol.py`
- [ ] Adicionar polling de estado `EXP_STAT` com estados específicos do experimento
- [ ] Implementar desserialização de `NASCERR_IMAGE_RESULT`
- [ ] Escrever classe controladora `nascerr_image_experiment.py`
- [ ] Exibição de tabela de métricas no console
- [ ] Registro de resultados em JSON

### Fase 3 — Modo de gatilho externo (laboratório de radiação)

- [ ] Implementar caminho de gatilho externo `delay_ms=0` no firmware
- [ ] Adicionar `CMD_IMAGE_RESULT_REQUEST` para acionar fase de leitura
- [ ] Adicionar estados EXP_STAT: `WRITE_DONE` e `READ_DONE`
- [ ] Testar fluxo de gatilho externo localmente com sabotador

### Fase 4 — Download completo de imagem e visualização

- [ ] Adicionar download paginado de dados de imagem ao firmware (leituras TXB indexadas por página)
- [ ] Adicionar `download_image_data()` ao controlador Python
- [ ] Implementar visualização lado a lado com `matplotlib`
- [ ] Adicionar exportação PNG para relatórios de laboratório

### Fase 5 — Conjunto estendido de imagens e análise de dados

- [ ] Adicionar outras imagens-padrão se necessário (Lena, cameraman, etc.)
- [ ] Análise estatística entre múltiplas repetições (intervalos de confiança)
- [ ] Geração automatizada de relatório (PDF ou LaTeX)

---

## 9. Decisões de Projeto e Justificativas

| Decisão | Escolha | Justificativa |
|---|---|---|
| Local da imagem-fonte | Flash interna do MCU (arrays const) | Já implementado, latência de leitura zero, sem complexidade de flash externa |
| Preenchimento da SRAM | Replicar imagem em N blocos | Maximiza células de memória em risco; N é configurável por teste |
| Estratégia de emparelhamento ECC | ECC selecionado + companheiro BYPASS automático | Comparação direta nas mesmas condições, um único comando por experimento |
| Separação escrita/leitura | Modo temporizado + modo de gatilho externo | Temporizador para testes locais, gatilho externo para laboratório de radiação |
| Transmissão de resultado | Apenas métricas (Fase 1), dados completos de imagem (Fase 2) | Métricas cabem em uma leitura TXB; dados de imagem precisam de paginação |
| Injeção de erros | Sabotador no firmware (macro `ERROR_ENABLE`) | Valida o pipeline completo sem equipamento de radiação |
| Padrão de compressão | `IMG_LOSSLESS` (sem compressão) | Comparação de pixels inequívoca; RLE é experimento secundário |

---

*Versão do documento: 1.0 — Planejamento inicial*  
*Autores: Danilo Alencar + Claude Sonnet 4.6*
