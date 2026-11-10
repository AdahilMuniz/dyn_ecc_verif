#!/usr/bin/env python3
"""
NASCERR Serial Logger
=====================
Captura saída serial do firmware e salva em arquivo para análise posterior
com nascerr_corruption_viewer.py.

Recursos:
  - Lista portas seriais disponíveis (--list)
  - Mirror em tempo real para stdout (com destaque para linhas de erro)
  - Timestamp por linha
  - Estatísticas em tempo real (linhas, erros, taxa)
  - Flush imediato no arquivo (resistente a crash/power loss)
  - Encerramento limpo via Ctrl+C com resumo

Uso:
  python nascerr_serial_logger.py --list
  python nascerr_serial_logger.py --port COM3
  python nascerr_serial_logger.py --port /dev/ttyUSB0 --baud 115200 -o sessao.log
  python nascerr_serial_logger.py --port COM3 --filter-errors
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from datetime import datetime
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Dependência ausente: pip install pyserial")
    sys.exit(1)


# ============================================================================
#  Constantes / formatação
# ============================================================================

# Mesma regex tolerante usada pelo corruption viewer
ERROR_PATTERN = re.compile(
    r"ENDERE.{1,8}0x[0-9A-Fa-f]+"
    r".+?MEM.{1,8}RIA\s+\S+?:\s*"
    r"[\d,\s]+?\s+"
    r"PADR.{1,8}0x[0-9A-Fa-f]+"
    r".+?LIDO.+?0x[0-9A-Fa-f]+",
)

# Detectores rápidos de outros tipos de linha interessantes.
# Cobre dois formatos: marcadores entre colchetes ([IMG_FLASH_READ], [IMG_ECC])
# e marcadores soltos no texto (EXP_READ_TASK, EXP_WRITE_TASK, HW_CHECK, etc).
INFO_PATTERN = re.compile(
    r"\[(IMG_FLASH_READ|IMG_FLASH_WRITE|IMG_ECC|IMG_BP|INFO|WARN|ERROR)\]"
    r"|EXP_(READ|WRITE)_TASK"
    r"|HW_CHECK|MEM_CHECK"
    r"|Command\s+received"
)
PSNR_PATTERN = re.compile(r"PSNR[_a-z]*\s*=\s*-?\d+\.\d+")

# Telemetria de HW (temperatura, correntes, potências) — gravada no arquivo,
# mas não espelhada no stdout (gera muito ruído no terminal).
HW_TELEMETRY_PATTERN = re.compile(r"\[HW\]\s*(T=|I\(mA\)|P\(mW\))")

# ANSI colors (funciona na maioria dos terminais modernos)
ANSI_RED    = "\033[91m"
ANSI_GREEN  = "\033[92m"
ANSI_YELLOW = "\033[93m"
ANSI_CYAN   = "\033[96m"
ANSI_DIM    = "\033[2m"
ANSI_RESET  = "\033[0m"


# ============================================================================
#  Helpers
# ============================================================================

def list_serial_ports() -> None:
    """Lista todas as portas seriais disponíveis no sistema."""
    ports = list_ports.comports()
    if not ports:
        print("Nenhuma porta serial encontrada.")
        return

    print(f"{'Porta':<20} {'Descrição':<40} {'HW ID'}")
    print("-" * 90)
    for p in ports:
        print(f"{p.device:<20} {(p.description or '-')[:40]:<40} {p.hwid}")


def classify_line(line: str) -> str:
    """Retorna o tipo da linha: 'error', 'info', 'psnr' ou 'other'."""
    if ERROR_PATTERN.search(line):
        return "error"
    if PSNR_PATTERN.search(line):
        return "psnr"
    if INFO_PATTERN.search(line):
        return "info"
    return "other"


def colorize(line: str, kind: str, no_color: bool) -> str:
    """Aplica cor ANSI dependendo do tipo de linha."""
    if no_color:
        return line
    if kind == "error":
        return f"{ANSI_RED}{line}{ANSI_RESET}"
    if kind == "psnr":
        return f"{ANSI_GREEN}{line}{ANSI_RESET}"
    if kind == "info":
        return f"{ANSI_CYAN}{line}{ANSI_RESET}"
    return f"{ANSI_DIM}{line}{ANSI_RESET}"


# ============================================================================
#  Loop de captura
# ============================================================================

def capture_loop(args: argparse.Namespace) -> None:
    out_path = Path(args.output)

    print(f"Abrindo porta {args.port} @ {args.baud} baud …")
    try:
        ser = serial.Serial(
            port=args.port,
            baudrate=args.baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1.0,  # leitura não-bloqueante (refresca stats periodicamente)
        )
    except serial.SerialException as e:
        print(f"Erro ao abrir porta: {e}")
        sys.exit(1)

    print(f"Conectado. Gravando em: {out_path}")
    print(f"Pressione Ctrl+C para encerrar.")
    print("-" * 70)

    # Abre arquivo em modo append, line-buffered (1) para flush a cada linha
    fp = open(out_path, "a", encoding="utf-8", errors="replace", buffering=1)

    # Cabeçalho da sessão no log
    session_start = datetime.now()
    fp.write(f"\n# ============================================================\n")
    fp.write(f"# NASCERR session  {session_start.isoformat(timespec='seconds')}\n")
    fp.write(f"# Port: {args.port}  Baud: {args.baud}\n")
    fp.write(f"# ============================================================\n")

    n_lines  = 0
    n_errors = 0
    n_psnr   = 0
    n_info   = 0
    last_status_t = time.monotonic()

    no_color = args.no_color or not sys.stdout.isatty()

    try:
        while True:
            # Lê uma linha bruta. Em caso de bytes inválidos, substitui.
            raw = ser.readline()
            if not raw:
                # Timeout sem dados — atualiza status (1× por segundo)
                now = time.monotonic()
                if now - last_status_t >= 1.0:
                    elapsed = now - last_status_t
                    if not args.quiet:
                        sys.stdout.write(
                            f"\r{ANSI_DIM}[{datetime.now().strftime('%H:%M:%S')}] "
                            f"linhas={n_lines}  erros={n_errors}  "
                            f"psnr={n_psnr}  info={n_info}{ANSI_RESET}   "
                        )
                        sys.stdout.flush()
                    last_status_t = now
                continue

            try:
                line = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            except Exception:
                line = repr(raw)

            if not line:
                continue

            n_lines += 1
            kind = classify_line(line)
            if   kind == "error": n_errors += 1
            elif kind == "psnr":  n_psnr   += 1
            elif kind == "info":  n_info   += 1

            # Filtro opcional: salva e mostra só linhas de erro
            if args.filter_errors and kind != "error":
                continue

            # Linha gravada no arquivo (com timestamp legível)
            ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            fp.write(f"{ts} | {line}\n")

            # Telemetria de HW só vai para o arquivo — não polui o terminal
            is_hw_telemetry = bool(HW_TELEMETRY_PATTERN.search(line))

            # Mirror para stdout
            if not args.quiet and not is_hw_telemetry:
                # Limpa linha de status atual
                sys.stdout.write("\r" + " " * 80 + "\r")
                colored = colorize(line, kind, no_color)
                ts_short = datetime.now().strftime("%H:%M:%S")
                print(f"{ANSI_DIM}[{ts_short}]{ANSI_RESET} {colored}")

    except KeyboardInterrupt:
        if not args.quiet:
            sys.stdout.write("\r" + " " * 80 + "\r")
        print("\nEncerrando captura …")

    finally:
        ser.close()
        session_end = datetime.now()
        duration = (session_end - session_start).total_seconds()

        # Rodapé no log
        fp.write(f"# ------------------------------------------------------------\n")
        fp.write(f"# Encerrada em {session_end.isoformat(timespec='seconds')}\n")
        fp.write(f"# Duração: {duration:.1f}s | "
                 f"linhas: {n_lines} | erros: {n_errors} | psnr: {n_psnr} | info: {n_info}\n")
        fp.write(f"# ============================================================\n\n")
        fp.close()

        # Resumo no stdout
        print()
        print(f"Sessão encerrada após {duration:.1f}s")
        print(f"  Linhas capturadas    : {n_lines}")
        print(f"  Erros (print_bit)    : {n_errors}")
        print(f"  Linhas com PSNR      : {n_psnr}")
        print(f"  Tasks/info markers   : {n_info}")
        print(f"  Arquivo salvo em     : {out_path}")
        if n_errors > 0:
            print()
            print("Próximo passo sugerido:")
            print(f"  python nascerr_corruption_viewer.py <imagem.bin> {out_path}")


# ============================================================================
#  Main
# ============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Captura logs serial do firmware NASCERR para análise posterior.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    parser.add_argument("--list",          action="store_true",
                        help="Lista portas seriais disponíveis e sai")
    parser.add_argument("--port",          default=None,
                        help="Porta serial (ex: COM3, /dev/ttyUSB0)")
    parser.add_argument("--baud",          type=int, default=115200,
                        help="Baud rate (padrão: 115200)")
    parser.add_argument("-o", "--output",  default=None,
                        help="Arquivo de saída (padrão: nascerr_<timestamp>.log)")
    parser.add_argument("--filter-errors", action="store_true",
                        help="Grava e mostra somente linhas de erro (print_bit_errors)")
    parser.add_argument("--no-color",      action="store_true",
                        help="Desabilita cores ANSI no stdout")
    parser.add_argument("-q", "--quiet",   action="store_true",
                        help="Não imprime nada no stdout, só grava no arquivo")

    args = parser.parse_args()

    if args.list:
        list_serial_ports()
        return

    if not args.port:
        print("Erro: --port é obrigatório (use --list para ver as disponíveis)")
        sys.exit(1)

    if args.output is None:
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        args.output = f"nascerr_{ts}.log"

    capture_loop(args)


if __name__ == "__main__":
    main()