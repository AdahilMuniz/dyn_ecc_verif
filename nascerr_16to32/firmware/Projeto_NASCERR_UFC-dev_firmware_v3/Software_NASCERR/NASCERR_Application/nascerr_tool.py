#!/usr/bin/env python3
"""
Ferramenta de linha de comando para interagir com o NASCERR via I²C.

Comandos disponíveis:
  i2cscan            – Varre o barramento I²C (usa i2cdetect -y 1)
  status             – Mostra o status geral do dispositivo
  boot-status        – Mostra informações de boot e versão do firmware
  expstatus          – Mostra o status do experimento (EXP_STAT)
  expsum-idx         – Define o offset da página de sumário (EXP_SUM_IDX)
  expsum             – Lê o sumário de experimentos (EXP_SUM_DATA)
  send               – Envia um único comando NASCERR_CMD
  send-preset        – Envia N comandos de teste sequenciais
  read-id            – Solicita dados de experimento por ID (READ_CMD)
  update-fw          – Realiza o processo de atualização de firmware
  read-idx           – Solicita dados por página/qtd_blocos (READ_CMD)
  read-exp           – Lê resultado(s) de experimento em TXB (get_experiment)
  clear-db           – Apaga região da flash (LOGS, ...)
  gps-data           - Gravos bytes de gps no NASCERR
"""
import argparse
import json
import subprocess
from typing import Union
from time import sleep

from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_protocol import Protocol
from nascerr_modules.nascerr_operation import Operation
from nascerr_modules.nascerr_system import FlashRegionTypeDef

# ---------------------------------------------------------------------------
# Constantes
# ---------------------------------------------------------------------------

SLEEP_TIME = 0.05   # Segundos
CONVERT_TIME = 1      # Fator minutos → ms (já usado em Operation)
MAX_WORD = 1024
MAX_PRESET = 67
MAX_PAGE = 191    # páginas válidas (0..191)

REGION_ALIAS = {
    # "header":   FlashRegionTypeDef.FLASH_HEADER,   # não permitido apagar remotamente
    "logs":     FlashRegionTypeDef.FLASH_LOGS,
    "scheduler": FlashRegionTypeDef.FLASH_SCHEDULER,
    "experiment": FlashRegionTypeDef.FLASH_EXPERIMENT,
    "backup":   FlashRegionTypeDef.FLASH_BACKUP_FIRMWARE,
}

# ---------------------------------------------------------------------------
# Funções utilitárias
# ---------------------------------------------------------------------------


def scan_i2c_bus() -> None:
    """Executa i2cdetect para listar dispositivos I²C na bus 1."""
    print("Executando i2cdetect -y 1...\n")
    try:
        output = subprocess.check_output(["i2cdetect", "-y", "1"], text=True)
        print(output)
    except FileNotFoundError:
        print("Erro: comando 'i2cdetect' não encontrado. Instale o pacote 'i2c-tools'.")
    except subprocess.CalledProcessError as exc:
        print(f"Erro ao executar i2cdetect: {exc}")


def show_status(op: Operation) -> None:
    print(json.dumps(op.get_status(), indent=4))


def show_boot_status(op: Operation) -> None:
    print(json.dumps(op.get_boot_status(), indent=4))


def show_experiment_status(op: Operation) -> None:
    print(json.dumps(op.get_experiment_status(), indent=4))


def set_expsum_idx(op: Operation, offset: int) -> None:
    resp = op.set_exp_sum_idx(offset)
    print(json.dumps(resp, indent=4))


def show_expsum(op: Operation, offset: Union[int, None]) -> None:
    if offset is not None:
        idx_resp = op.set_exp_sum_idx(offset)
        print(">> set_exp_sum_idx:", json.dumps(idx_resp, indent=4))
        sleep(SLEEP_TIME)
    sum_resp = op.get_exp_sum()
    print(json.dumps(sum_resp, indent=4))


def send_command(op: Operation, args: argparse.Namespace) -> None:
    result = op.send_commandNew(
        cmd_type=CmdTypeDef.WRITE_CMD if args.write else CmdTypeDef.READ_CMD,
        id=args.cmd_id,
        block=args.block,
        qtd_block=args.qtd,
        ecc=NASCERR_MODE.MRSC,
        write_type=W_DataTypeDef.WRITE_xAAh,
        scheduler_time=args.scheduler,
        repeat=args.repeat,
        delay=args.delay,
        current_tick=0,
    )
    print(json.dumps(result, indent=4))


def send_predefined_commands(op: Operation, qtd: int) -> None:
    qtd = min(max(1, qtd), MAX_PRESET)
    print(f"########## ENVIO DE {qtd} COMANDOS ##########")
    for blk in range(1, qtd + 1):
        result = op.send_commandNew(
            cmd_type=CmdTypeDef.WRITE_CMD,
            id=blk,
            block=blk,
            qtd_block=1,
            ecc=NASCERR_MODE.MRSC,
            write_type=W_DataTypeDef.WRITE_xAAh,
            scheduler_time=1_000 + blk * 1_000,
            repeat=1,
            delay=1_000 + blk * 1_000,
            current_tick=0,
        )
        print(f"Cmd #{blk:02d} ->", json.dumps(result, indent=4))
        sleep(0.1)


def read_experiment_by_id(op: Operation, args: argparse.Namespace) -> None:
    if not 0 <= args.page <= MAX_PAGE:
        print(json.dumps({"error": True, "status": "fail",
              "log_error": f"page deve estar entre 0 e {MAX_PAGE}"}, indent=4))
        return

    result = op.send_commandNew(
        cmd_type=CmdTypeDef.READ_CMD,
        id=0,
        block=args.page,
        qtd_block=args.qtd_block,
        ecc=NASCERR_MODE.MRSC,
        write_type=0,
        scheduler_time=0,
        repeat=0,
        delay=0,
        current_tick=0,
    )
    print(json.dumps(result, indent=4))

def handle_firmware_update(op: Operation, args: argparse.Namespace) -> None:
    """Chama o processo completo de atualização de firmware."""
    print(f"Iniciando atualização do firmware com o arquivo: {args.file}")
    print(f"Nova versão: {args.version:#08x}")
    try:
        update_result = op.update_firmware(
            firmware_path=args.file,
            new_version=args.version
        )
        print("\n>> Resultado da transferência:")
        print(json.dumps(update_result, indent=4))
    except FileNotFoundError:
        print(f"\nERRO: O arquivo de firmware '{args.file}' não foi encontrado.")
    except Exception as e:
        print(f"\nERRO: Ocorreu uma falha durante a atualização: {e}")

def read_experiment(op: Operation) -> None:
    resp = op.read_experiment()
    print(json.dumps(resp, indent=4))


def clear_database(op: Operation, region_key: str):
    region = REGION_ALIAS[region_key]
    print(json.dumps(op.clear_database(region), indent=4))


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Ferramenta de interação com o NASCERR via I²C")
    sub = parser.add_subparsers(dest="cmd")

    sub.add_parser("i2cscan", help="Escaneia dispositivos no barramento I²C")
    sub.add_parser("status", help="Mostra o status do dispositivo")
    sub.add_parser(
        "boot-status", help="Mostra informações do bootloader e firmware")
    sub.add_parser(
        "expstatus", help="Mostra o status dos experimentos (EXP_STAT)")

    idx = sub.add_parser(
        "expsum-idx", help="Define o offset (0,30,60,…) da página em EXP_SUM_IDX")
    idx.add_argument("--offset", type=int, required=True,
                     help="Offset múltiplo de 30 (0-255)")

    expsum = sub.add_parser(    
        "expsum", help="Lê EXP_SUM_DATA (opcionalmente define o offset antes)")
    expsum.add_argument("--offset", type=int, default=None,
                        help="Offset opcional para setar antes da leitura")

    preset = sub.add_parser(
        "send-preset", help="Envia sequência de comandos predefinidos (1-67)")
    preset.add_argument("--qtd", type=int, default=64,
                        help="Quantidade de comandos (1-67)")

    snd = sub.add_parser("send", help="Envia um comando individual")
    snd.add_argument("--cmd_id", type=int, required=True)
    snd.add_argument("--block", type=int, required=True)
    snd.add_argument("--qtd", type=int, default=1)
    snd.add_argument("--scheduler", type=int, default=100_000)
    snd.add_argument("--repeat", type=int, default=1)
    snd.add_argument("--delay", type=int, default=100_000)
    snd.add_argument("--write", action="store_true")

    read_idx = sub.add_parser(
        "read-idx", help="Lê experimento(s) por página/qtd_blocos (READ_CMD)")
    read_idx.add_argument("--page", type=int, default=0, help="Página 0-191")
    read_idx.add_argument("--qtd_block", type=int, default=0,
                         help="Qtd de blocos (0=todos)")

    sub.add_parser(
        "read-exp", help="Lê resultado(s) em TXB via get_experiment")

    sp_clr = sub.add_parser(
        "clear-db", help="Apaga região da flash (exceto header)")
    sp_clr.add_argument("--region", choices=list(REGION_ALIAS.keys()))

    sub.add_parser("gps-data",  help="Grava 38 bytes 0xAA no registrador GPS")

    update_fw = sub.add_parser(
        "update-fw",
        help="Envia um novo firmware para o dispositivo"
    )
    update_fw.add_argument(
        "--file",
        type=str,
        required=True,
        help="Caminho para o arquivo .bin do firmware"
    )
    update_fw.add_argument(
        "--version",
        type=lambda x: int(x, 0), # Permite entrada em hex (0x...) ou dec
        required=True,
        help="Nova versão do firmware (ex: 0x00010008)"
    )

    return parser


def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()

    device_name = "NASCERR_CargaUtil"
    i2c_address = 0x55

    op = Operation(device_name, i2c_address)
    pro = Protocol("Nascerr", i2c_address)

    if args.cmd == "i2cscan":
        scan_i2c_bus()
    elif args.cmd == "status":
        show_status(op)
    elif args.cmd == "boot-status":
        show_boot_status(op)
    elif args.cmd == "expstatus":
        show_experiment_status(op)
    elif args.cmd == "expsum-idx":
        set_expsum_idx(op, args.offset)
    elif args.cmd == "expsum":
        show_expsum(op, args.offset)
    elif args.cmd == "send":
        send_command(op, args)
    elif args.cmd == "send-preset":
        send_predefined_commands(op, args.qtd)
    elif args.cmd == "update-fw":
        handle_firmware_update(op, args)
    elif args.cmd == "read-id":
        if hasattr(pro.i2c, "restart_i2c_bus"):
            pro.i2c.restart_i2c_bus()
    elif args.cmd == "read-idx":
        read_experiment_by_id(op, args)
    elif args.cmd == "read-exp":
        read_experiment(op)
    elif args.cmd == "clear-db":
        clear_database(op, args.region)
    elif args.cmd == "gps-data":
        print(json.dumps(op.gps_data(), indent=4))
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
