#!/usr/bin/env python3
"""
NASCERR Corruption Viewer
=========================
Reconstroi visualmente uma imagem corrompida na SRAM a partir de:
  - O .bin original gerado por nascerr_img_prepare.py (com header NSCR)
  - O log serial do firmware contendo as linhas print_bit_errors

A ferramenta cruza as informações:
  - .bin    → imagem original + layout dos kernels
  - log     → posições/valores das palavras corrompidas

E produz quatro visualizações:
  1. Imagem original
  2. Imagem corrompida (pixels lidos com erro substituídos)
  3. Mapa de magnitude do erro absoluto (|corrompido - original|)
  4. Heatmap espacial da localização dos erros

Uso:
  python nascerr_corruption_viewer.py original.bin log.txt
  python nascerr_corruption_viewer.py original.bin log.txt -o saida.png
"""

from __future__ import annotations

import argparse
import math
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

try:
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm
except ImportError:
    print("Dependência ausente: pip install matplotlib")
    sys.exit(1)


# ============================================================================
#  De-permutação ECC (inverso do encoder coluna-maior do TBEC/MRSC)
# ============================================================================

def depermute_data(permuted: int) -> int:
    """
    Inverte a permutação coluna-maior do encoder TBEC/MRSC.
    permuted: 16 bits de dados como armazenados no BYPASS_1.
    Retorna o par de pixels original (16 bits).

    Encoder layout (idêntico em TBEC e MRSC):
      bits 31..16 = {signal[0][0]..signal[3][0],   <- col 0 (MSB→)
                     signal[0][1]..signal[3][1],   <- col 1
                     signal[0][2]..signal[3][2],   <- col 2
                     signal[0][3]..signal[3][3]}   <- col 3
      signal[r][c] = bit (15 - r*4 - c) do pixel original
    """
    matrix = [[0]*4 for _ in range(4)]
    bit_idx = 15
    for c in range(4):
        for r in range(4):
            matrix[r][c] = (permuted >> bit_idx) & 1
            bit_idx -= 1
    result = 0
    for r in range(4):
        for c in range(4):
            result |= matrix[r][c] << (15 - r*4 - c)
    return result


# ============================================================================
#  Parser do .bin (formato NSCR)
# ============================================================================

NSCR_MAGIC = b"NSCR"
NSCR_HEADER_LEN = 16  # 4 (magic) + 6*2 (uint16 BE fields)


@dataclass
class BinInfo:
    img_w: int
    img_h: int
    kernel_w: int
    kernel_h: int
    n_kx: int
    n_ky: int

    @property
    def kernel_size(self) -> int:
        return self.kernel_w * self.kernel_h

    @property
    def total_data_bytes(self) -> int:
        return self.n_kx * self.n_ky * self.kernel_size


def parse_bin(path: Path) -> tuple[BinInfo, bytes]:
    """Lê o .bin do nascerr_img_prepare e retorna (info, dados_kernels)."""
    raw = path.read_bytes()
    if len(raw) < NSCR_HEADER_LEN:
        raise ValueError(f"Arquivo .bin muito pequeno ({len(raw)} bytes)")
    if raw[:4] != NSCR_MAGIC:
        raise ValueError(f"Magic inválido: {raw[:4]!r}, esperado {NSCR_MAGIC!r}")

    fields = struct.unpack(">HHHHHH", raw[4:NSCR_HEADER_LEN])
    info = BinInfo(*fields)
    data = raw[NSCR_HEADER_LEN:]
    return info, data


# ============================================================================
#  Mapeamento byte → pixel (layout de kernels)
# ============================================================================

def byte_pos_to_pixel(byte_pos: int, info: BinInfo) -> tuple[int, int] | None:
    """
    Converte uma posição de byte na seção de dados do .bin em coordenada
    (x, y) na imagem original. Retorna None se o byte cai em padding ou
    fora dos limites.
    """
    kernel_index = byte_pos // info.kernel_size
    in_kernel = byte_pos % info.kernel_size

    if kernel_index >= info.n_kx * info.n_ky:
        return None

    kx = kernel_index % info.n_kx
    ky = kernel_index // info.n_kx

    px_in_k = in_kernel % info.kernel_w
    py_in_k = in_kernel // info.kernel_w

    x = kx * info.kernel_w + px_in_k
    y = ky * info.kernel_h + py_in_k

    if x >= info.img_w or y >= info.img_h:
        return None  # área de padding

    return (x, y)


def reconstruct_original(data: bytes, info: BinInfo) -> np.ndarray:
    """Reconstroi a imagem original a partir dos dados de kernels do .bin."""
    img = np.zeros((info.img_h, info.img_w), dtype=np.uint8)
    for b_pos, byte_val in enumerate(data):
        pixel = byte_pos_to_pixel(b_pos, info)
        if pixel is not None:
            x, y = pixel
            img[y, x] = byte_val
    return img


# ============================================================================
#  Parser do log do firmware
# ============================================================================

# Regex tolerante a problemas de encoding (Ã£o, Ã‡, …).
# Captura: (address_hex, mem_name, bit_positions, expected_hex, read_hex)
LOG_PATTERN = re.compile(
    r"ENDERE.{1,8}0x([0-9A-Fa-f]+)"        # endereço (word index)
    r".+?MEM.{1,8}RIA\s+(\S+?):\s*"        # nome da memória
    r"([\d,\s]+?)\s+"                       # posições dos bits
    r"PADR.{1,8}0x([0-9A-Fa-f]+)"          # valor esperado
    r".+?LIDO.+?0x([0-9A-Fa-f]+)",          # valor lido
)

# Regex para captura do status ECC na linha imediatamente seguinte
ECC_STATUS_PATTERN = re.compile(
    r"\[ECC:\s*(CORRIGIDO|NAO\s+CORRIGIDO|N.O\s+CORRIGIDO|DIVERG.NCIA[^\]]*)\]",
    re.IGNORECASE,
)


@dataclass
class LogError:
    word_index: int        # índice da palavra na SRAM (sram_i)
    mem_name: str          # SRAM1, SRAM2, SRAM_ECC, SRAM_BP
    bit_positions: list[int]
    expected: int          # 16 ou 32 bits dependendo do contexto
    read_value: int
    is_32bit: bool         # True se o log é da pipeline ECC
    ecc_status: str | None = None  # "corrected", "uncorrected", "divergence" ou None


def parse_log(path: Path, sram_base: int = 0x60000000) -> list[LogError]:
    """Extrai todas as ocorrências de erro do log serial.

    sram_base - endereço base da SRAM reportado no log (e.g. 0x60000000).
                É subtraído de cada endereço capturado para obter o offset
                relativo ao início da região de dados.

    Para entradas ECC (SRAM_ECC), captura também o status [ECC: ...] da
    linha imediatamente seguinte, quando presente.
    """
    errors: list[LogError] = []
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()

    i = 0
    while i < len(lines):
        line = lines[i]
        m = LOG_PATTERN.search(line)
        if not m:
            i += 1
            continue

        addr_hex, mem_name, bits_str, exp_hex, read_hex = m.groups()
        bit_positions = [int(b) for b in re.findall(r"\d+", bits_str)]
        expected = int(exp_hex, 16)
        read_value = int(read_hex, 16)

        is_32bit = expected > 0xFFFF or read_value > 0xFFFF or mem_name == "SRAM_ECC"

        # Para ECC, tenta capturar o [ECC: ...] da próxima linha
        ecc_status: str | None = None
        if is_32bit and i + 1 < len(lines):
            ms = ECC_STATUS_PATTERN.search(lines[i + 1])
            if ms:
                token = ms.group(1).upper().replace(" ", "")
                if "CORRIGIDO" in token and "NAO" not in token and "NÃO" not in token:
                    ecc_status = "corrected"
                elif "NAOCORRIGIDO" in token or "NÃOCORRIGIDO" in token:
                    ecc_status = "uncorrected"
                elif "DIVERG" in token:
                    ecc_status = "divergence"
                i += 1  # consome a linha de status

        raw_addr = int(addr_hex, 16)
        word_index = (raw_addr - sram_base) // 2  # converte byte-offset relativo → word index

        errors.append(LogError(
            word_index=word_index,
            mem_name=mem_name,
            bit_positions=bit_positions,
            expected=expected,
            read_value=read_value,
            is_32bit=is_32bit,
            ecc_status=ecc_status,
        ))
        i += 1

    return errors


# ============================================================================
#  Aplicação dos erros sobre a imagem
# ============================================================================

def detect_firmware_header_offset(errors: list[LogError]) -> int:
    """
    Auto-detecta se o firmware escreveu o header NSCR junto com os pixels.
    Retorna o offset em bytes (0 = sem header, 16 = header NSCR presente).

    Se word 0 no log tem PADRÃO 0x4E53 ('NS' = magic NSCR), o firmware
    gravou os 16 bytes do header do .bin como se fossem pixels.
    """
    for err in errors[:10]:
        if err.is_32bit:
            continue
        if err.word_index == 0 and err.expected == 0x4E53:
            return 16  # header NSCR detectado
    return 0


def apply_errors(original: np.ndarray,
                 errors: list[LogError],
                 info: BinInfo,
                 firmware_header_bytes: int = 0
                 ) -> tuple[np.ndarray, np.ndarray, dict]:
    """
    Aplica cada erro do log sobre uma cópia da imagem.

    Parâmetros:
      firmware_header_bytes - quantos bytes o firmware escreveu antes dos
                              pixels reais (16 se gravou o header NSCR junto).

    Retorna:
      corrupted   - imagem com pixels afetados substituídos pelo valor lido
      error_map   - magnitude do erro absoluto por pixel (uint8 saturado)
      stats       - dicionário com estatísticas agregadas
    """
    corrupted = original.copy()
    error_accum = np.zeros_like(original, dtype=np.int32)

    n_pixels_affected = 0
    n_out_of_bounds   = 0
    n_in_header       = 0
    n_ecc_errors      = 0
    sum_abs_diff      = 0

    for err in errors:
        # Pipeline ECC (32 bits): log é da codeword permutada, não dos pixels
        if err.is_32bit:
            n_ecc_errors += 1
            continue

        word_idx = err.word_index
        exp_hi  = (err.expected   >> 8) & 0xFF
        exp_lo  =  err.expected         & 0xFF
        read_hi = (err.read_value >> 8) & 0xFF
        read_lo =  err.read_value       & 0xFF

        # Subtrai o cabeçalho que o firmware gravou antes dos pixels
        hi_byte_pos = word_idx * 2     - firmware_header_bytes
        lo_byte_pos = word_idx * 2 + 1 - firmware_header_bytes

        for byte_pos, exp_b, read_b in (
            (hi_byte_pos, exp_hi, read_hi),
            (lo_byte_pos, exp_lo, read_lo),
        ):
            if exp_b == read_b:
                continue

            if byte_pos < 0:
                # Erro caiu no header NSCR — não corresponde a pixel algum
                n_in_header += 1
                continue

            pixel = byte_pos_to_pixel(byte_pos, info)
            if pixel is None:
                n_out_of_bounds += 1
                continue

            x, y = pixel
            corrupted[y, x] = read_b
            error_accum[y, x] += abs(int(read_b) - int(exp_b))
            sum_abs_diff += abs(int(read_b) - int(exp_b))
            n_pixels_affected += 1

    error_map = np.clip(error_accum, 0, 255).astype(np.uint8)

    diff = corrupted.astype(np.int32) - original.astype(np.int32)
    mse = float(np.mean(diff * diff))
    psnr = float("inf") if mse == 0.0 else 10.0 * math.log10(255.0 * 255.0 / mse)

    stats = {
        "n_log_errors":          len(errors),
        "n_ecc_skipped":         n_ecc_errors,
        "n_pixels_affected":     n_pixels_affected,
        "n_out_of_bounds":       n_out_of_bounds,
        "n_in_header":           n_in_header,
        "firmware_header_bytes": firmware_header_bytes,
        "avg_abs_diff":          (sum_abs_diff / n_pixels_affected) if n_pixels_affected else 0.0,
        "mse":                   mse,
        "psnr":                  psnr,
    }

    return corrupted, error_map, stats


# ============================================================================
#  Análise ECC: de-permutação + status do decoder
# ============================================================================

def apply_errors_ecc(original: np.ndarray,
                     errors: list[LogError],
                     info: BinInfo,
                     firmware_header_bytes: int = 0
                     ) -> tuple[np.ndarray, np.ndarray, np.ndarray, dict]:
    """
    Análise para o modo ECC (TBEC/MRSC).

    Para cada entrada do log com 32 bits (SRAM_ECC):
      - Extrai os 16 bits altos da codeword (data bank permutado)
      - Aplica depermute_data para obter o par de pixels "data_raw"
      - Localiza os pixels alvo via mapeamento word→byte→pixel

    Constrói três imagens:
      pre_ecc       — imagem com pixels do data_raw (degradação física sem ECC)
      post_ecc      — imagem após o ECC: corrigidos = original; falhas = data_raw
                       (aproxima o que o FPGA realmente entregou nos pontos onde falhou)
      effectiveness — mapa categórico:
                       0 = sem erro
                       1 = ECC corrigiu (verde)
                       2 = ECC falhou (vermelho)
                       3 = divergência FPGA/SW (laranja)
    """
    pre_ecc       = original.copy()
    post_ecc      = original.copy()
    effectiveness = np.zeros_like(original, dtype=np.uint8)

    n_corrected    = 0
    n_uncorrected  = 0
    n_divergence   = 0
    n_unknown      = 0   # entradas ECC sem [ECC: ...] na linha seguinte
    n_pixels_pre   = 0   # pixels marcados na imagem pré-ECC
    n_pixels_post  = 0   # pixels visualmente errados na imagem pós-ECC
    n_in_header    = 0
    n_out_of_bounds = 0

    for err in errors:
        if not err.is_32bit:
            continue  # entradas bypass (16 bits) não pertencem a essa análise

        # Extrai apenas os 16 bits altos (data bank) das codewords de 32 bits
        exp_data_perm  = (err.expected   >> 16) & 0xFFFF
        read_data_perm = (err.read_value >> 16) & 0xFFFF

        # Desfaz a permutação para voltar ao espaço do pixel
        exp_pixels  = depermute_data(exp_data_perm)
        read_pixels = depermute_data(read_data_perm)

        exp_hi,  exp_lo  = (exp_pixels  >> 8) & 0xFF, exp_pixels  & 0xFF
        read_hi, read_lo = (read_pixels >> 8) & 0xFF, read_pixels & 0xFF

        # Status ECC desta entrada
        status = err.ecc_status
        if status is None:
            n_unknown += 1

        if   status == "corrected":   n_corrected   += 1
        elif status == "uncorrected": n_uncorrected += 1
        elif status == "divergence":  n_divergence  += 1

        word_idx = err.word_index
        hi_byte_pos = word_idx * 2     - firmware_header_bytes
        lo_byte_pos = word_idx * 2 + 1 - firmware_header_bytes

        for byte_pos, exp_b, read_b in (
            (hi_byte_pos, exp_hi, read_hi),
            (lo_byte_pos, exp_lo, read_lo),
        ):
            if exp_b == read_b:
                continue  # este pixel do par não foi atingido

            if byte_pos < 0:
                n_in_header += 1
                continue

            pixel = byte_pos_to_pixel(byte_pos, info)
            if pixel is None:
                n_out_of_bounds += 1
                continue

            x, y = pixel

            # Imagem pré-ECC: sempre usa o valor degradado (data_raw)
            pre_ecc[y, x] = read_b
            n_pixels_pre += 1

            # Imagem pós-ECC depende do status do decoder
            if status == "corrected":
                # FPGA entregou o valor original → imagem permanece intacta
                effectiveness[y, x] = 1
            elif status == "uncorrected":
                # FPGA não conseguiu corrigir → assume data_raw como saída
                post_ecc[y, x]   = read_b
                effectiveness[y, x] = 2
                n_pixels_post   += 1
            elif status == "divergence":
                # FPGA e SW discordam → marca como divergência
                post_ecc[y, x]   = read_b
                effectiveness[y, x] = 3
                n_pixels_post   += 1
            else:
                # Status desconhecido — trata como degradação visível
                post_ecc[y, x]   = read_b
                effectiveness[y, x] = 2
                n_pixels_post   += 1

    # Métricas
    def calc_psnr(img: np.ndarray) -> float:
        d   = img.astype(np.int32) - original.astype(np.int32)
        mse = float(np.mean(d * d))
        return float("inf") if mse == 0.0 else 10.0 * math.log10(255.0 * 255.0 / mse)

    psnr_pre  = calc_psnr(pre_ecc)
    psnr_post = calc_psnr(post_ecc)

    n_total = n_corrected + n_uncorrected + n_divergence + n_unknown

    stats = {
        "n_log_errors_ecc":       n_total,
        "n_corrected":            n_corrected,
        "n_uncorrected":          n_uncorrected,
        "n_divergence":           n_divergence,
        "n_unknown_status":       n_unknown,
        "n_pixels_pre":           n_pixels_pre,
        "n_pixels_post":          n_pixels_post,
        "n_in_header":            n_in_header,
        "n_out_of_bounds":        n_out_of_bounds,
        "firmware_header_bytes":  firmware_header_bytes,
        "psnr_pre":               psnr_pre,
        "psnr_post":              psnr_post,
        "ecc_gain_db":            (psnr_post - psnr_pre) if psnr_pre != float("inf") else 0.0,
        "correction_efficiency":  (n_corrected / n_total) if n_total else 0.0,
    }

    return pre_ecc, post_ecc, effectiveness, stats


# ============================================================================
#  Visualização
# ============================================================================

def plot_results(original: np.ndarray,
                 corrupted: np.ndarray,
                 error_map: np.ndarray,
                 info: BinInfo,
                 stats: dict,
                 output: Path) -> None:
    """
    Layout 2×3 para o modo bypass.

      Original              | Corrompida          | |Corrompida − Original|
      Subtração com sinal   | Localização         | Histograma de erro
    """
    fig, axes = plt.subplots(2, 3, figsize=(16, 10))

    # === Row 1 ==============================================================

    # 1. Original
    axes[0, 0].imshow(original, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    axes[0, 0].set_title(f"Original ({info.img_w}×{info.img_h})")
    axes[0, 0].axis("off")

    # 2. Corrompida
    axes[0, 1].imshow(corrupted, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    axes[0, 1].set_title(f"Corrompida  (PSNR = {stats['psnr']:.2f} dB)")
    axes[0, 1].axis("off")

    # 3. Subtração absoluta |corrompida − original|
    diff_abs = np.abs(corrupted.astype(np.int32) - original.astype(np.int32)).astype(np.uint8)
    vmax_abs = max(int(diff_abs.max()), 1)
    im3 = axes[0, 2].imshow(diff_abs, cmap="hot", vmin=0, vmax=vmax_abs, interpolation="nearest")
    axes[0, 2].set_title("|Corrompida − Original|  (magnitude)")
    axes[0, 2].axis("off")
    plt.colorbar(im3, ax=axes[0, 2], fraction=0.046, pad=0.04)

    # === Row 2 ==============================================================

    # 4. Subtração com sinal (corrompida − original), centrada em 0
    diff_signed = corrupted.astype(np.int32) - original.astype(np.int32)
    vmax_sgn = max(abs(int(diff_signed.min())), abs(int(diff_signed.max())), 1)
    im4 = axes[1, 0].imshow(diff_signed, cmap="RdBu_r",
                             vmin=-vmax_sgn, vmax=vmax_sgn, interpolation="nearest")
    axes[1, 0].set_title("Corrompida − Original  (direção do erro)")
    axes[1, 0].axis("off")
    plt.colorbar(im4, ax=axes[1, 0], fraction=0.046, pad=0.04)

    # 5. Localização espacial dos erros
    location = (diff_abs > 0).astype(np.uint8)
    axes[1, 1].imshow(original, cmap="gray", vmin=0, vmax=255, interpolation="nearest", alpha=0.4)
    axes[1, 1].imshow(np.ma.masked_where(location == 0, location),
                       cmap="autumn", interpolation="nearest", alpha=0.9)
    axes[1, 1].set_title(f"Localização dos erros ({int(location.sum())} pixels)")
    axes[1, 1].axis("off")
    for kx in range(1, info.n_kx):
        axes[1, 1].axvline(kx * info.kernel_w - 0.5, color="cyan", linewidth=0.4, alpha=0.5)
    for ky in range(1, info.n_ky):
        axes[1, 1].axhline(ky * info.kernel_h - 0.5, color="cyan", linewidth=0.4, alpha=0.5)

    # 6. Histograma do erro com sinal
    nonzero_diff = diff_signed[diff_signed != 0]
    if nonzero_diff.size > 0:
        bins = min(80, int(2 * vmax_sgn) + 1)
        axes[1, 2].hist(nonzero_diff.ravel(), bins=bins, color="#1976D2",
                        edgecolor="white", linewidth=0.3)
        axes[1, 2].axvline(0, color="black", linewidth=0.6, alpha=0.5)
        axes[1, 2].set_title("Distribuição dos erros (pixels afetados)")
        axes[1, 2].set_xlabel("Δ = corrompida − original")
        axes[1, 2].set_ylabel("nº de pixels")
        axes[1, 2].grid(alpha=0.3, linewidth=0.5)
    else:
        axes[1, 2].text(0.5, 0.5, "Nenhum erro detectado",
                         ha="center", va="center", fontsize=11,
                         transform=axes[1, 2].transAxes)
        axes[1, 2].axis("off")

    # Subtítulo
    subtitle = (
        f"Kernels {info.kernel_w}×{info.kernel_h} ({info.n_kx}×{info.n_ky} = "
        f"{info.n_kx * info.n_ky})  |  "
        f"Log: {stats['n_log_errors']} erros  |  "
        f"Pixels afetados: {stats['n_pixels_affected']}  |  "
        f"Δ médio: {stats['avg_abs_diff']:.1f}"
    )
    if stats["firmware_header_bytes"]:
        subtitle += f"  |  FW header: {stats['firmware_header_bytes']}B"
    if stats["n_in_header"]:
        subtitle += f"  |  No header: {stats['n_in_header']}"
    if stats["n_out_of_bounds"]:
        subtitle += f"  |  Fora dos limites: {stats['n_out_of_bounds']}"
    if stats["n_ecc_skipped"]:
        subtitle += f"  |  ECC (ignorados): {stats['n_ecc_skipped']}"

    fig.suptitle(subtitle, fontsize=10)
    plt.tight_layout(rect=(0, 0, 1, 0.96))
    plt.savefig(output, dpi=150, bbox_inches="tight")
    print(f"Figura salva em: {output}")


def plot_results_ecc(original: np.ndarray,
                     pre_ecc: np.ndarray,
                     post_ecc: np.ndarray,
                     effectiveness: np.ndarray,
                     info: BinInfo,
                     stats: dict,
                     output: Path) -> None:
    """
    Layout 2×3 para o modo ECC.

      Original             | Pré-ECC             | Pós-ECC
      |Pré − Original|     | |Pós − Original|    | Efetividade
    """
    from matplotlib.colors import ListedColormap
    from matplotlib.patches import Patch

    fig, axes = plt.subplots(2, 3, figsize=(16, 10))

    # === Row 1: imagens reconstruídas ======================================

    axes[0, 0].imshow(original, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    axes[0, 0].set_title(f"Original ({info.img_w}×{info.img_h})")
    axes[0, 0].axis("off")

    axes[0, 1].imshow(pre_ecc, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    axes[0, 1].set_title(f"Pré-ECC (data_raw)  PSNR = {stats['psnr_pre']:.2f} dB")
    axes[0, 1].axis("off")

    psnr_post_str = "∞" if stats['psnr_post'] == float("inf") else f"{stats['psnr_post']:.2f} dB"
    axes[0, 2].imshow(post_ecc, cmap="gray", vmin=0, vmax=255, interpolation="nearest")
    axes[0, 2].set_title(f"Pós-ECC  PSNR = {psnr_post_str}")
    axes[0, 2].axis("off")

    # === Row 2: subtrações + efetividade ===================================
    #
    # Para os dois |diff| compartilharmos a MESMA escala de cor: assim a
    # comparação visual entre Pré e Pós é direta — se o Pós aparece mais
    # escuro/menos saturado, o ECC reduziu a magnitude do erro.
    diff_pre  = np.abs(pre_ecc.astype(np.int32)  - original.astype(np.int32)).astype(np.uint8)
    diff_post = np.abs(post_ecc.astype(np.int32) - original.astype(np.int32)).astype(np.uint8)
    vmax_shared = max(int(diff_pre.max()), int(diff_post.max()), 1)

    im1 = axes[1, 0].imshow(diff_pre, cmap="hot", vmin=0, vmax=vmax_shared, interpolation="nearest")
    axes[1, 0].set_title(f"|Pré-ECC − Original|  (n={int((diff_pre > 0).sum())})")
    axes[1, 0].axis("off")
    plt.colorbar(im1, ax=axes[1, 0], fraction=0.046, pad=0.04)

    im2 = axes[1, 1].imshow(diff_post, cmap="hot", vmin=0, vmax=vmax_shared, interpolation="nearest")
    axes[1, 1].set_title(f"|Pós-ECC − Original|  (n={int((diff_post > 0).sum())})")
    axes[1, 1].axis("off")
    plt.colorbar(im2, ax=axes[1, 1], fraction=0.046, pad=0.04)

    # Efetividade do ECC (categórico)
    cmap_eff = ListedColormap(["#00000000", "#00C853", "#D32F2F", "#FF6F00"])
    axes[1, 2].imshow(original, cmap="gray", vmin=0, vmax=255, interpolation="nearest", alpha=0.35)
    axes[1, 2].imshow(effectiveness, cmap=cmap_eff, vmin=0, vmax=3,
                       interpolation="nearest", alpha=0.95)
    axes[1, 2].set_title("Efetividade do ECC")
    axes[1, 2].axis("off")

    for kx in range(1, info.n_kx):
        axes[1, 2].axvline(kx * info.kernel_w - 0.5, color="cyan", linewidth=0.4, alpha=0.5)
    for ky in range(1, info.n_ky):
        axes[1, 2].axhline(ky * info.kernel_h - 0.5, color="cyan", linewidth=0.4, alpha=0.5)

    legend_items = [
        Patch(color="#00C853", label=f"Corrigido ({stats['n_corrected']})"),
        Patch(color="#D32F2F", label=f"Não corrigido ({stats['n_uncorrected']})"),
        Patch(color="#FF6F00", label=f"Divergência FPGA/SW ({stats['n_divergence']})"),
    ]
    if stats["n_unknown_status"]:
        legend_items.append(Patch(color="#888888", label=f"Status ausente ({stats['n_unknown_status']})"))
    axes[1, 2].legend(handles=legend_items, loc="upper right", fontsize=8, framealpha=0.85)

    # Subtítulo
    eff  = stats["correction_efficiency"] * 100.0
    gain = stats["ecc_gain_db"]
    gain_str = "∞" if stats["psnr_post"] == float("inf") else f"{gain:+.2f}"

    subtitle = (
        f"Kernels {info.kernel_w}×{info.kernel_h} ({info.n_kx}×{info.n_ky})  |  "
        f"ECC entries: {stats['n_log_errors_ecc']}  |  "
        f"Eficiência ECC: {eff:.1f}%  |  "
        f"Ganho PSNR: {gain_str} dB"
    )
    if stats["firmware_header_bytes"]:
        subtitle += f"  |  FW header: {stats['firmware_header_bytes']}B"
    if stats["n_in_header"]:
        subtitle += f"  |  No header: {stats['n_in_header']}"
    if stats["n_out_of_bounds"]:
        subtitle += f"  |  Fora dos limites: {stats['n_out_of_bounds']}"

    fig.suptitle(subtitle, fontsize=10)
    plt.tight_layout(rect=(0, 0, 1, 0.96))
    plt.savefig(output, dpi=150, bbox_inches="tight")
    print(f"Figura salva em: {output}")


# ============================================================================
#  Main
# ============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Reconstrução visual de corrupção de imagem na SRAM "
                    "a partir do log serial do firmware NASCERR.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("bin",  help="Arquivo .bin original (formato NSCR)")
    parser.add_argument("log",  help="Arquivo de log serial do firmware")
    parser.add_argument("-o", "--output", default="corruption.png",
                        help="Caminho da figura de saída (padrão: corruption.png)")
    parser.add_argument("--firmware-header-bytes", type=int, default=None,
                        help="Quantos bytes o firmware gravou na SRAM antes dos pixels "
                             "(16 se inclui o header NSCR). Padrão: auto-detecta pelo log.")
    parser.add_argument("--sram-base", default="0x60000000",
                        help="Endereço base da SRAM no log (padrão: 0x60000000). "
                             "Use 0x00000000 se o log já reporta offsets relativos.")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Imprime todos os erros parseados")
    args = parser.parse_args()

    bin_path = Path(args.bin)
    log_path = Path(args.log)
    out_path = Path(args.output)

    if not bin_path.exists():
        print(f"Erro: .bin não encontrado: {bin_path}"); sys.exit(1)
    if not log_path.exists():
        print(f"Erro: log não encontrado: {log_path}"); sys.exit(1)

    # 1. Parse do .bin
    info, data = parse_bin(bin_path)
    print(f".bin lido: {info.img_w}×{info.img_h}, "
          f"kernels {info.kernel_w}×{info.kernel_h}, "
          f"grid {info.n_kx}×{info.n_ky}")

    # 2. Reconstroi a imagem original
    original = reconstruct_original(data, info)
    print(f"Imagem original reconstruída: {original.shape}")

    # 3. Parse do log
    sram_base = int(args.sram_base, 0)
    errors = parse_log(log_path, sram_base=sram_base)
    print(f"Erros lidos do log: {len(errors)}")

    # 3.5. Detecta/define offset do firmware
    if args.firmware_header_bytes is None:
        firmware_header_bytes = detect_firmware_header_offset(errors)
        if firmware_header_bytes > 0:
            print(f"⚠ Cabeçalho NSCR detectado na SRAM: word 0 = 0x4E53 ('NS')")
            print(f"  Aplicando offset de {firmware_header_bytes} bytes automaticamente.")
            print(f"  (use --firmware-header-bytes 0 para desabilitar)")
    else:
        firmware_header_bytes = args.firmware_header_bytes
        if firmware_header_bytes > 0:
            print(f"Offset do header firmware: {firmware_header_bytes} bytes (manual)")

    if args.verbose:
        for e in errors:
            ecc_tag = f" [{e.ecc_status}]" if e.ecc_status else ""
            print(f"  word=0x{e.word_index:08X}  mem={e.mem_name}  "
                  f"bits={e.bit_positions}  "
                  f"exp=0x{e.expected:0{8 if e.is_32bit else 4}X}  "
                  f"read=0x{e.read_value:0{8 if e.is_32bit else 4}X}{ecc_tag}")

    # 4. Detecta modo: ECC se houver qualquer entrada de 32 bits
    n_ecc_entries    = sum(1 for e in errors if e.is_32bit)
    n_bypass_entries = sum(1 for e in errors if not e.is_32bit)

    if n_ecc_entries > 0 and n_bypass_entries == 0:
        print(f"Modo detectado: ECC ({n_ecc_entries} entradas de 32 bits)")
        pre_ecc, post_ecc, effectiveness, stats = apply_errors_ecc(
            original, errors, info, firmware_header_bytes=firmware_header_bytes)

        print()
        print("Estatísticas (modo ECC):")
        print(f"  Entradas ECC          : {stats['n_log_errors_ecc']}")
        print(f"    Corrigidas          : {stats['n_corrected']}")
        print(f"    Não corrigidas      : {stats['n_uncorrected']}")
        print(f"    Divergência FPGA/SW : {stats['n_divergence']}")
        print(f"    Status ausente      : {stats['n_unknown_status']}")
        print(f"  Pixels pré-ECC        : {stats['n_pixels_pre']}")
        print(f"  Pixels pós-ECC errados: {stats['n_pixels_post']}")
        print(f"  No header (ignorados) : {stats['n_in_header']}")
        print(f"  Fora dos limites      : {stats['n_out_of_bounds']}")
        print(f"  Eficiência ECC        : {stats['correction_efficiency']*100:.2f}%")
        print(f"  PSNR pré-ECC          : {stats['psnr_pre']:.2f} dB")
        psnr_post_str = "∞" if stats['psnr_post'] == float("inf") else f"{stats['psnr_post']:.2f} dB"
        print(f"  PSNR pós-ECC          : {psnr_post_str}")
        print()

        plot_results_ecc(original, pre_ecc, post_ecc, effectiveness,
                         info, stats, out_path)
        return

    # Bypass / 16-bit
    print(f"Modo detectado: BYPASS ({n_bypass_entries} entradas de 16 bits)")
    corrupted, error_map, stats = apply_errors(
        original, errors, info, firmware_header_bytes=firmware_header_bytes)

    print()
    print("Estatísticas (modo BYPASS):")
    print(f"  Erros no log         : {stats['n_log_errors']}")
    print(f"  Pixels afetados      : {stats['n_pixels_affected']}")
    print(f"  No header (ignorados): {stats['n_in_header']}")
    print(f"  Fora dos limites     : {stats['n_out_of_bounds']}")
    print(f"  Skipped (ECC mode)   : {stats['n_ecc_skipped']}")
    print(f"  Δ médio por pixel    : {stats['avg_abs_diff']:.2f}")
    print(f"  MSE                  : {stats['mse']:.2f}")
    print(f"  PSNR                 : {stats['psnr']:.2f} dB")
    print()

    plot_results(original, corrupted, error_map, info, stats, out_path)


if __name__ == "__main__":
    main()