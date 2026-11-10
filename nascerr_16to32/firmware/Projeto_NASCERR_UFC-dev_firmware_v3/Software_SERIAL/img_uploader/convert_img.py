#!/usr/bin/env python3
"""
NASCERR Image Prepare
=====================
Converte uma imagem para escala de cinza, divide em kernels (blocos)
de tamanho configurável e salva tudo em um arquivo .bin.

Layout do .bin:
  HEADER (16 bytes):
    - magic          : 4B  "NSCR"
    - img_width      : 2B  (big-endian)
    - img_height     : 2B  (big-endian)
    - kernel_w       : 2B  (big-endian)
    - kernel_h       : 2B  (big-endian)
    - n_kernels_x    : 2B  (big-endian)
    - n_kernels_y    : 2B  (big-endian)

  DADOS:
    - kernels concatenados em row-major order (esquerda→direita, cima→baixo)
    - cada kernel tem kernel_w * kernel_h bytes (1 byte por pixel, grayscale)
    - pixels fora da imagem original (padding) são preenchidos com 0

Uso:
  python convert_img.py foto.png                        # kernels 64x64 (padrão)
  python convert_img.py foto.png -k 32                  # kernels 32x32
  python convert_img.py foto.png -k 64x48               # kernels 64 largura x 48 altura
  python convert_img.py foto.png -k 64 -o saida.bin     # nome de saída customizado
"""

import argparse
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Dependência ausente: pip install Pillow")
    sys.exit(1)


MAGIC = b"NSCR"


def parse_kernel_size(s: str) -> tuple[int, int]:
    """Aceita '64' (quadrado) ou '64x48' (largura x altura)."""
    s = s.strip().lower()
    if "x" in s:
        parts = s.split("x", 1)
        w, h = int(parts[0]), int(parts[1])
    else:
        w = h = int(s)
    if w <= 0 or h <= 0:
        raise ValueError("Tamanho do kernel deve ser > 0")
    return w, h


def image_to_kernels(img_path: str, kernel_w: int, kernel_h: int) -> tuple[bytes, dict]:
    """
    Abre a imagem, converte para grayscale, divide em kernels e retorna
    (dados_bin, info_dict).
    """
    img = Image.open(img_path).convert("L")  # grayscale
    img_w, img_h = img.size

    # Quantidade de kernels em cada eixo (arredonda pra cima)
    n_kx = (img_w + kernel_w - 1) // kernel_w
    n_ky = (img_h + kernel_h - 1) // kernel_h

    pixels = img.load()
    kernels_data = bytearray()

    for ky in range(n_ky):
        for kx in range(n_kx):
            for py in range(kernel_h):
                for px in range(kernel_w):
                    x = kx * kernel_w + px
                    y = ky * kernel_h + py
                    if x < img_w and y < img_h:
                        kernels_data.append(pixels[x, y])
                    else:
                        kernels_data.append(0)  # padding

    info = {
        "img_w": img_w,
        "img_h": img_h,
        "kernel_w": kernel_w,
        "kernel_h": kernel_h,
        "n_kx": n_kx,
        "n_ky": n_ky,
        "total_kernels": n_kx * n_ky,
        "kernel_bytes": kernel_w * kernel_h,
        "total_bytes": len(kernels_data),
    }

    # Monta header
    header = MAGIC + struct.pack(
        ">HHHHHH",
        img_w, img_h,
        kernel_w, kernel_h,
        n_kx, n_ky,
    )

    return bytes(header) + bytes(kernels_data), info


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Converte imagem → grayscale → kernels → .bin",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Exemplos:\n"
            "  python nascerr_img_prepare.py foto.png\n"
            "  python nascerr_img_prepare.py foto.png -k 32\n"
            "  python nascerr_img_prepare.py foto.png -k 64x48\n"
            "  python nascerr_img_prepare.py foto.png -k 64 -o saida.bin\n"
        ),
    )
    parser.add_argument("image", help="Caminho da imagem de entrada")
    parser.add_argument(
        "-k", "--kernel", default="64",
        help="Tamanho do kernel: '64' (quadrado) ou '64x48' (LxA). Padrão: 64",
    )
    parser.add_argument(
        "-o", "--output", default=None,
        help="Caminho do .bin de saída (padrão: <nome_imagem>.bin)",
    )

    args = parser.parse_args()

    img_path = Path(args.image)
    if not img_path.exists():
        print(f"Erro: arquivo não encontrado: {img_path}")
        sys.exit(1)

    try:
        kernel_w, kernel_h = parse_kernel_size(args.kernel)
    except ValueError as e:
        print(f"Erro no tamanho do kernel: {e}")
        sys.exit(1)

    out_path = Path(args.output) if args.output else img_path.with_suffix(".bin")

    print(f"Imagem:  {img_path}")
    print(f"Kernel:  {kernel_w}x{kernel_h}")

    data, info = image_to_kernels(str(img_path), kernel_w, kernel_h)

    out_path.write_bytes(data)

    print(f"Resolução original: {info['img_w']}x{info['img_h']}")
    print(f"Grid de kernels:    {info['n_kx']} x {info['n_ky']} = {info['total_kernels']} kernels")
    print(f"Bytes por kernel:   {info['kernel_bytes']}")
    print(f"Header:             {len(MAGIC) + 12} bytes")
    print(f"Dados:              {info['total_bytes']} bytes")
    print(f"Total:              {len(data)} bytes")
    print(f"Salvo em:           {out_path}")


if __name__ == "__main__":
    main()