import json
from time import sleep
from random import randint, choice
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_operation import Operation

SLEEP_TIME = 0.2

cmd_queue_max_len = 64

cmd_execution_qtd = 0

# ira sendo incrementado a medida que um cmd write for enviado
identificador = 0

qtd_page = 0

##################### Tratamento dos blocos disponiveis #####################

"""
    Estrutura que representa os blocos da sram do firmware
"""
block = set()
for i in range(0, 64):
    block.add(i)


def allocate_block() -> int:
    """
    Seleciona um bloco livre e tenta alocar sequencialmente uma quantidade de blocos.

    • A quantidade de blocos a alocar é escolhida aleatoriamente no intervalo [1, blk_range]  
    • Se algum bloco do intervalo já estiver ocupado, a alocação é truncada até o último válido  
    • Caso não haja blocos disponíveis, retorna (-1, -1)  
    • Retorna (ini_index, qtd_block) onde:
        – ini_index = índice inicial do bloco alocado  
        – qtd_block = quantidade efetivamente alocada
    """

    blk_range = 1

    if len(block) == 0:
        # retorno padrao de tudo ja ta alocado
        return -1, -1

    # escolhe necessariamente um bloco livre do conjunto
    ini_index = choice(list(block))
    # escolhe uma qtd_aleatoria, mas nao necessariamente eh
    qtd_block = randint(1, blk_range)

    final_index = ini_index + qtd_block

    if final_index > 64:
        final_index = 64

    qtd_block = 0

    for i in range(ini_index, final_index):
        if i not in block:
            return ini_index, qtd_block

        qtd_block = qtd_block + 1

        block.remove(i)

    return ini_index, qtd_block


def deallocate_block(start_block: int, qtd_block: int) -> None:
    """
    Libera blocos previamente alocados, reinserindo-os no conjunto `block`.

    • start_block = índice inicial do intervalo de blocos a desalocar  
    • qtd_block   = quantidade de blocos a liberar a partir de start_block  
    • Os índices no intervalo [start_block, start_block + qtd_block) são adicionados novamente a `block`  
    • Não retorna valor (apenas atualiza o conjunto de blocos livres)
    """

    # Ainda nao integrada a logica
    for i in range(start_block, start_block+qtd_block):
        block.add(i)


#####################  Logica do produtor #####################

def produce(op: Operation) -> bool:
    """
    Gera e envia um comando de escrita para o experimento, com parâmetros aleatórios.

    • Seleciona blocos livres por meio de `allocate_block()`, respeitando colisões  
    • Se não houver blocos disponíveis, retorna False  
    • Define aleatoriamente:
        – ecc: modo de correção de erros (MRSC ou TBEC)  
        – write_type: padrão de escrita (0x55, 0xAA, 0xFF, 0x00)  
        – scheduler_time: instante de início do experimento (ms)  
        – repeat: número de repetições (0 = sem repetições)  
        – delay: intervalo entre repetições (igual ao scheduler_time)  
    • Envia o comando via `op.send_commandNew()` com os parâmetros definidos  
    • Incrementa os contadores globais `identificador` e `qtd_page`  
    • Retorna True em caso de envio bem-sucedido
    """
    blk, qtd_blk = allocate_block()

    # caso em que nao a mais espaco livre
    if blk == -1 or qtd_blk == -1:
        return False

    # escolha aleatoria dos parametros
    global identificador

    ecc = choice([NASCERR_MODE.MRSC, NASCERR_MODE.TBEC])

    write_type = choice([W_DataTypeDef.WRITE_x55h, W_DataTypeDef.WRITE_ONES,
                        W_DataTypeDef.WRITE_ZEROS, W_DataTypeDef.WRITE_xAAh])

    scheduler_time = randint(2000, 3000)

    delay = scheduler_time

    repeat = randint(0, 0)

    # Envio do comando
    print(f"[WRITE_CMD] id={identificador}, block={blk}, qtd_block={qtd_blk}")
    comando = op.send_commandNew(
        cmd_type=CmdTypeDef.WRITE_CMD,
        id=identificador,
        block=blk,
        qtd_block=qtd_blk,
        ecc=ecc,
        write_type=write_type,
        scheduler_time=scheduler_time,
        repeat=repeat,
        delay=delay,                         # delay igual ao scheduler_time
        current_tick=0
    )
    # print(f"Cmd #{identificador:02d} ->", json.dumps(comando, indent=4))
    sleep(SLEEP_TIME)
    # vai aumentando a medida que novo exp e feito
    identificador = identificador + 1

    # utilizado controle da qtd comando
    global qtd_page
    qtd_page = qtd_page + repeat + 1
    print(qtd_page)
    return True

#####################  Logica do consumidor #####################


def get_unread_pages(op: Operation) -> set:
    """
    Percorre sequencialmente as páginas do sumário de experimentos e retorna
    o conjunto de páginas não lidas.

    • Usa `set_exp_sum_idx(offset)` para selecionar cada página (30 itens por pagina) 
    • A posição global de cada entrada é dada por (offset + index)  
    • O campo `status` indica se a página foi lida ou não  
    • O campo `id` não é usado como índice, apenas como informação adicional  
    • Continua enquanto `qtd_included > 0` e offset ≤ 210  
    • Em caso de falha ou exceção, retorna um conjunto vazio
    """

    unread = set()
    offset = 0
    resp = None  # guarda última resposta para debug

    try:
        # primeira consulta
        sleep(SLEEP_TIME)
        error_set = op.set_exp_sum_idx(0)
        sleep(SLEEP_TIME)
        if error_set.get("error") is True:
            print("Erro ao setar página:", error_set.get("data"))

        # Verificação dados página 0
        resp = op.get_exp_sum()
        if resp.get("error") is True:
            print("Erro ao obter sumário:", resp.get("data"))
            return set()

        data = resp["data"]
        entries = data.get("entries", [])

        if entries:
            # print(json.dumps(data, indent=4))
            sleep(SLEEP_TIME)

        while data["qtd_included"] > 0:
            for index, entry in enumerate(entries):
                if entry["status"] == "NOT_READ":
                    unread.add(index + offset)

            offset += 30
            if offset > 210:
                print("mais paginas que suportado")
                break

            error_set = op.set_exp_sum_idx(offset)
            sleep(SLEEP_TIME)
            if error_set.get("error") is True:
                print("Erro ao setar página:", error_set.get("data"))
                return set()

            resp = op.get_exp_sum()
            if resp.get("error") is True:
                print("Erro ao obter sumário:", resp.get("data"))
                return set()

            data = resp["data"]
            entries = data.get("entries", [])
            # print(json.dumps(data, indent=4))
            sleep(SLEEP_TIME)

    except Exception as e:
        # print(f"[ERROR] get_unread_pages: {e}")
        return set()  # retorna vazio em caso de falha

    return unread


def read_experiment(op: Operation) -> dict:
    """
    Realiza a leitura de um experimento, falha caso nenhum exista, lido ou nao.
    """
    resp = op.read_experiment()
    if resp["error"]:
        print("read_experiment")
        print(resp["log_error"])
    print("[READ_EXP DATA]:")
    print(json.dumps(resp, indent=4))
    return resp


def read_experiment_page(op: Operation, page: int) -> dict:
    """
    Realiza a leitura de uma página específica de um experimento.

    • O parâmetro `page` define diretamente qual página do experimento será lida  
    • O comando enviado é do tipo `READ_CMD`, com `block=page`  
    • Demais parâmetros (`qtd_block`, `ecc`, `write_type`, `scheduler_time`, 
      `repeat`, `delay`, `current_tick`) são fixados em 0 ou valores padrão,
      pois não influenciam na leitura  

    """
    resp = op.send_commandNew(
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
    if resp["error"]:
        print("read_experiment_page")
        print(resp["log_error"])
    return resp


def consume(op: Operation) -> bool:
    """
    Consome experimentos pendentes lendo todas as páginas não lidas.

    • Obtém a lista de páginas não lidas com `get_unread_pages()`  
    • Como o resultado é um `set` (não ordenável), aplica `sorted()` para garantir 
      leitura em ordem crescente de páginas  
    • A ideia é sempre priorizar as páginas de menor índice e ler na sequência correta  
    • Para cada página:
        – Envia comando de leitura com `read_experiment_page(page)`  
        – Em seguida, chama `read_experiment()` para obter o conteúdo  
    • Em caso de sucesso (sem erro em `result_page` e `result_read`), decrementa `qtd_page`   
    • Atualiza a lista de não lidos a cada iteração até que não restem páginas  
    • Retorna True se processou pelo menos uma página, False se não havia nada a ler
    """

    global qtd_page
    print("pages to read:", qtd_page)

    unread_pages = get_unread_pages(op)
    sleep(SLEEP_TIME)

    if len(unread_pages) == 0:
        return False

    while len(unread_pages) > 0:
        unread_pages = sorted(list(unread_pages))
        page = unread_pages.pop(0)
        # print("reading: ", page)
        # print("pages to read:", qtd_page)
        print(f"[READ_PAGE] {page}")
        result_page = read_experiment_page(op, page)
        sleep(SLEEP_TIME)
        result_read = read_experiment(op)
        sleep(SLEEP_TIME)

        if not (result_page["error"] or result_read["error"]):
            qtd_page = qtd_page - 1
            print("pages to read:", qtd_page)
        else:
            print("error get page or read")

        unread_pages = get_unread_pages(op)
        sleep(SLEEP_TIME)

    return True


def main():
    device_name = "NASCERR_CargaUtil"
    i2c_address = 0x55
    op = Operation(device_name, i2c_address)
    global qtd_page

    # 1. Mostra status inicial
    status = op.get_status()
    print("STATUS INICIAL:")
    print(json.dumps(status, indent=4))
    sleep(SLEEP_TIME)

    # 2. Cria um primeiro experimento
    print("\n[PRODUCE] Criando experimento...")
    produce(op)

    # 3. Loop até não restar página pendente
    while qtd_page > 0:
        sleep(SLEEP_TIME)

        # Verifica páginas não lidas
        unread = get_unread_pages(op)
        print("\n[UNREAD PAGES]:", unread)

        if unread:
            print("[CONSUME] Lendo páginas não lidas...")
            consume(op)
        else:
            print("[PRODUCE] Criando novo experimento...")
            produce(op)


# Suspeito erro exp_sum - sempre que ultilizado gera erros


if __name__ == "__main__":
    main()
