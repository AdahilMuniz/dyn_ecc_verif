import argparse
import json
import os
from time import sleep
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_protocol import Protocol
from nascerr_modules.nascerr_operation import Operation

STATE_FILE = "nascerr_state.json"


# ------------------------------
# Funções utilitárias p/ arquivo
# ------------------------------
def load_state():
    if not os.path.exists(STATE_FILE):
        return {"meta": {"last_page": 0}, "unread": {}}
    with open(STATE_FILE, "r") as f:
        return json.load(f)


def save_state(state):
    with open(STATE_FILE, "w") as f:
        json.dump(state, f, indent=2)


def add_experiment(exp_id: int, num_pages: int):
    state = load_state()
    last_page = state["meta"]["last_page"]

    pages = list(range(last_page, last_page + num_pages))
    state["meta"]["last_page"] = last_page + num_pages
    state["unread"][str(exp_id)] = pages

    save_state(state)
    print(f"[MAPPER] ID={exp_id} → páginas {pages}")


def remove_experiment(exp_id: int):
    state = load_state()
    if str(exp_id) in state["unread"]:
        del state["unread"][str(exp_id)]
        save_state(state)


def clear_state():
    """Reseta o estado local (apaga páginas não lidas e reinicia last_page)."""
    state = {"meta": {"last_page": 0}, "unread": {}}
    save_state(state)
    print("[STATE] Estado local foi limpo (last_page=0, unread vazio).")


# ------------------------------
# Operações CLI
# ------------------------------
def show_unread():
    state = load_state()
    if not state["unread"]:
        print("[UNREAD] Nenhum resultado pendente.")
        return
    for exp_id, pages in state["unread"].items():
        print(json.dumps(
            {"id": int(exp_id), "rep_faltando": len(pages), "pages": pages},
            indent=2
        ))


def consume_unread(op: Operation):
    state = load_state()
    if not state["unread"]:
        print("[CONSUME] Nenhum resultado disponível.")
        return

    for exp_id, pages in list(state["unread"].items()):
        print(f"[CONSUME] ID={exp_id}, páginas pendentes={pages}")

        for page in pages:
            print(f"  -> Consumindo página {page}")
            intermediario = op.send_commandNew(
                cmd_type=CmdTypeDef.READ_CMD,
                id=int(exp_id),
                block=page,
                qtd_block=1,
                ecc=NASCERR_MODE.MRSC,
                write_type=0,
                scheduler_time=0,
                repeat=0,
                delay=0,
                current_tick=0,
            )
            print("     READ_CMD:", json.dumps(intermediario, indent=2))
            sleep(0.1)

            resp = op.read_experiment()
            print(f"     Resultado página {page}:")
            print(json.dumps(resp, indent=2))
            sleep(0.1)

        # remove após consumir
        remove_experiment(exp_id)


# ------------------------------
# CLI
# ------------------------------
def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Ferramenta de interação com o NASCERR via I²C")
    sub = parser.add_subparsers(dest="cmd")

    loop = sub.add_parser("loop", help="Loop interativo para enviar comandos")
    loop.add_argument("--scheduler", type=int, default=10000)
    loop.add_argument("--delay", type=int, default=10000)

    sub.add_parser("show-unread", help="Mostra IDs de experimentos não lidos")
    sub.add_parser("consume-unread", help="Consome experimentos não lidos")
    sub.add_parser("clear-state", help="Reseta o estado local (apaga arquivo JSON)")

    return parser


def main():
    parser = build_arg_parser()
    args = parser.parse_args()

    device_name = "NASCERR_CargaUtil"
    i2c_address = 0x55
    op = Operation(device_name, i2c_address)
    pro = Protocol("Nascerr", i2c_address)

    if args.cmd == "loop":
        print("Loop iniciado. Digite 'exit' para parar o envio.")
        while True:
            try:
                cmd = input("\nDigite o ID do experimento (ou 'exit'): ")
                if cmd.strip().lower() == "exit":
                    print("[INFO] Loop de envio interrompido. Use show-unread ou consume-unread.")
                    break

                cmd_id = int(cmd)
                block = int(input("Digite o número do bloco: "))

                resultado = op.send_commandNew(
                    cmd_type=CmdTypeDef.WRITE_CMD,
                    id=cmd_id,
                    block=block,
                    qtd_block=1,
                    ecc=NASCERR_MODE.TBEC,
                    write_type=W_DataTypeDef.WRITE_xAAh,
                    scheduler_time=args.scheduler,
                    repeat=1,
                    delay=args.delay,
                    current_tick=0
                )
                print("[PRODUCER] Comando enviado:", json.dumps(resultado, indent=2))

                # Atualiza o arquivo de estado
                add_experiment(cmd_id, num_pages=2)  # repeat=1 → 2 páginas

            except KeyboardInterrupt:
                print("\n[INFO] Ctrl+C ignorado (use 'exit' para sair).")
                continue
            except Exception as e:
                print("[PRODUCER] Erro:", e)

    elif args.cmd == "show-unread":
        show_unread()

    elif args.cmd == "consume-unread":
        consume_unread(op)

    elif args.cmd == "clear-state":
        clear_state()

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
