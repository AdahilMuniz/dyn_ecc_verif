import json
from time import sleep
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_operation import Operation

SLEEP_TIME = 0.2   # tempo de respiro entre I2C ops


# ============================================================
# Auxiliares de alocação de blocos
# ============================================================

def allocate_blocks(total_experiments: int, start_block: int = 0):
    """
    Aloca blocos de forma sequencial (em ordem crescente).
    Exemplo: allocate_blocks(3, start=0) -> [(0,1), (1,1), (2,1)]
    """
    result = []
    blk = start_block
    for i in range(total_experiments):
        result.append((blk, 1))  # cada experimento ocupa 1 bloco
        blk += 1
    return result


# ============================================================
# PRODUCE: escreve novos experimentos
# ============================================================

def produce_experiment(op: Operation, exp_id: int, block: int, qtd_block: int) -> dict:
    """
    Envia um WRITE_CMD para criar um novo experimento no bloco indicado.
    """
    ecc = NASCERR_MODE.MRSC if exp_id % 2 == 0 else NASCERR_MODE.TBEC
    write_type = W_DataTypeDef.WRITE_x55h if exp_id % 2 == 0 else W_DataTypeDef.WRITE_xAAh

    comando = op.send_commandNew(
        cmd_type=CmdTypeDef.WRITE_CMD,
        id=exp_id,
        block=block,
        qtd_block=qtd_block,
        ecc=ecc,
        write_type=write_type,
        scheduler_time=1,
        repeat=0,
        delay=0,
        current_tick=0
    )
    print(f"[WRITE_CMD] id={exp_id}, block={block}, qtd_block={qtd_block}, ecc={ecc.name}")
    return comando


# ============================================================
# FUNÇÃO: consultar sumário
# ============================================================

def get_unread_pages(op: Operation) -> set:
    """
    Percorre o EXP_SUM usando EXP_SUM_IDX e retorna páginas com status NOT_READ.
    """
    unread = set()
    offset = 0

    while True:
        # define página de sumário
        op.set_exp_sum_idx(offset)
        sleep(SLEEP_TIME)

        resp = op.get_exp_sum()
        if resp["error"]:
            break

        data = resp["data"]
        entries = data.get("entries", [])

        # print(json.dumps(resp, indent=4))

        for idx, entry in enumerate(entries):
            if entry["status"] == "NOT_READ":
                unread.add(offset + idx)

        # parar se não houver mais itens incluídos
        if data["qtd_included"] == 0:
            break

        offset += 30
        if offset > 210:
            break

    return unread

def consume_pages(op: Operation):
    """
    Consulta EXP_SUM, lê páginas não lidas e mostra os resultados.
    """
    pending_reads = set()  # páginas já solicitadas

    while True:
        unread = sorted(list(get_unread_pages(op)))

        # remove da lista local aquelas que já foram confirmadas como lidas
        unread = [p for p in unread if p not in pending_reads]

        if not unread and not pending_reads:
            print("Nenhuma página pendente para leitura.")
            break

        for page in unread:
            print(f"[READ_PAGE] {page}")
            resp_op = op.send_commandNew(
                cmd_type=CmdTypeDef.READ_CMD,
                id=0,
                block=page,
                qtd_block=0,
                ecc=NASCERR_MODE.MRSC,
                write_type=0,
                scheduler_time=0,
                repeat=0,
                delay=0,
                current_tick=0,
            )
            # print(">> send_commandNew:", json.dumps(resp_op, indent=4))

            # marca como pendente até o firmware confirmar
            pending_reads.add(page)
            sleep(SLEEP_TIME)
            result = op.read_experiment()
            print(json.dumps(result, indent=4))
        # Verifica se alguma pendente já foi marcada como lida
        latest_unread = get_unread_pages(op)
        pending_reads = {p for p in pending_reads if p in latest_unread}

        sleep(SLEEP_TIME)


def wait_for_experiments(op: Operation, timeout=30):
    """
    Polling: espera até que pelo menos 1 experimento fique pronto para leitura.
    """
    for t in range(timeout):
        status = op.get_experiment_status()
        data = status["data"]

        pend = data["qtd_scheduler_pending"]
        unread = data["qtd_experiments_unread"]

        print(f"[STATUS] pend={pend}, unread={unread}")

        if unread > 0:
            print("Experimentos disponíveis para leitura!")
            return True

        sleep(1.0)  # aguarda 1s antes de checar novamente

    print("Timeout: nenhum experimento ficou pronto no tempo limite.")
    return False


def main():
    device_name = "NASCERR_CargaUtil"
    i2c_address = 0x55
    op = Operation(device_name, i2c_address)

    # status inicial
    status = op.get_status()
    print("STATUS INICIAL:")
    print(json.dumps(status, indent=4))

    # enfileirar 5 experimentos
    total_experiments = 50
    for exp_id, (blk, qtd) in enumerate(allocate_blocks(total_experiments, start_block=0)):
        produce_experiment(op, exp_id, blk, qtd)

    if wait_for_experiments(op, timeout=60):
        consume_pages(op)


if __name__ == "__main__":
    main()
