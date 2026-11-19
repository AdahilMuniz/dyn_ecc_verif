from datetime import datetime
from typing import List
from fastapi import Depends, FastAPI, Request
from fastapi.responses import RedirectResponse, FileResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
from helpers import CommandEntry, ResponseEntry, CommandFormModel, IDGenerator, StatusResponse, PersistentIDManager
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_image_processing import *
from nascerr_modules.nascerr_operation import Operation
from nascerr_modules.util.csv import ExperimentResultSaver
from nascerr_modules.util.db import LoggerJSON
from nascerr_modules.nascerr_system import OPERATION_LOGS
from starlette.background import BackgroundTask
from fastapi import BackgroundTasks
import asyncio
import os
import math
import random
from fastapi.responses import JSONResponse
import time

app = FastAPI()


#DB
db = LoggerJSON()

# Configurar o Jinja2 para renderizar os templates HTML
templates = Jinja2Templates(directory="nascerr_template")
# Caminhos corretos para templates e arquivos estáticos
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
static_dir = os.path.join(BASE_DIR, "nascerr_template", "static")
templates_dir = os.path.join(BASE_DIR, "nascerr_template")

# Monta o diretório estático somente se ele existir (evita erro em testes)
if os.path.isdir(static_dir):
    app.mount("/static", StaticFiles(directory=static_dir), name="static")

# Configuração do Jinja2
templates = Jinja2Templates(directory=templates_dir)

# app.mount("/static", StaticFiles(directory="nascerr_template/static"), name="static")

# # Configurar o Jinja2 para renderizar os templates HTML
# templates = Jinja2Templates(directory="nascerr_template")

# Gerar ID incrementall (DELETAR DEPOIS DE DB)
# id_gen = IDGenerator()
id_gen = PersistentIDManager()

# Initialize the protocol for the device
device_name = "NASCERR_CargaUtil"
i2c_address = 0x55
operacao = Operation("NASCERR_CargaUtil ", 0x55)

MEM_LIM = 0x200000  

# Vetor para armazenar os comandos
command_list: List[CommandEntry] = []
# Vetor para armazenar os resultados das operações
operation_results: List[ResponseEntry] = []

#estrutura para armazenar o status da operação contínua
continuous_status = {
    "running": False,
    "current": 0,
    "total": 0
}

stop_requested = False

@app.get("/")
async def index(request: Request):
    cmd_options = [(cmd.name, cmd.value) for cmd in CmdTypeDef]
    write_options = [(w.name, w.value) for w in W_DataTypeDef]
    read_options = [(seq.name, seq.value) for seq in Seq_ReadTypeDef]
    ecc_options = [(ecc.name, ecc.value) for ecc in NASCERR_MODE]

    # Renderizar o template HTML 'enviar_comando.html'
    return templates.TemplateResponse("enviar_comando.html", {
        "request": request,
        "cmd_options": cmd_options,
        "write_options": write_options,
        "read_options": read_options,
        "ecc_options": ecc_options,
        "command_list": command_list,
    })

@app.get("/status", response_model=StatusResponse)
async def get_status_endpoint(background_tasks: BackgroundTasks): 

    if continuous_status["running"]:
        return {
            "status": f"Executando experimento {continuous_status['current']} de {continuous_status['total']}"
        }
    status_st = operacao.get_status()
    
    if status_st["error"] == False :
        if status_st["data"]["operation"]["name"] == "OP_TRANSMIT_RESPONSE" or status_st["data"]["operation"]["name"] == "OP_TRANSMIT_FINISH":
            # Verifica se há um pedido para transmitir o resultado
            if status_st["data"]["request"]["name"] == "REQ_TXRESULT":
                result = operacao.read_result_byPackages()
                # print(result)
                if result["error"]:
                    print("Erro ao obter o resultado")
                else:
                    if math.isinf(result["data"].get("percent_error", 0)):
                        result["data"]["percent_error"] = None  
                    operation_results.append(ResponseEntry(
                        response=result,
                        receive_date=datetime.now(),
                        status='SUCCESS'
                    ))
                    db.save_results_json(operation_results)

        elif status_st["data"]["operation"]["name"] == "OP_EXPERIMENT" or status_st["data"]["operation"]["name"] == "OP_RECEIVE_COMMAND" or status_st["data"]["operation"]["name"]=="OP_TRANSMIT_RESPONSE" or status_st["data"]["operation"]["name"]=="OP_TRANSMIT_RESPONSE_FINISH":
            operation_name = status_st["data"]["operation"]["name"]
            try:
                op_enum = OPERATION_LOGS[operation_name]
                description = op_enum.description
            except KeyError:
                description = operation_name 
            return {"status": description}
            
    status_info = {"status": status_st["connection_status"]}
    return status_info


@app.get("/receber_resultado")
async def receber_resultado(request: Request):
    return templates.TemplateResponse("receber_resultado.html", {
        "request": request, 
        "result_list": operation_results
    })

@app.get("/results")
async def get_results():
    return [entry.to_dict() for entry in operation_results]

@app.post("/new_command")
async def new_command(background_tasks: BackgroundTasks, request: Request,
                      command: CommandFormModel = Depends(CommandFormModel.as_form)):

    nascerr_cmd = NASCERR_CTRL(
        cmd_type=command.cmd_type,
        id=id_gen.generate_id(),
        data_lenght=command.data_length if command.data_length is not None else 0,  # Fornecendo valor padrão
        repeat=command.repeat if command.repeat is not None else 1,                 # Fornecendo valor padrão
        write_type=command.write_type if command.write_type is not None else 0,     # Fornecendo valor padrão
        data=command.read if command.read is not None else 1,                       # Fornecendo valor padrão
        mode=command.ecc if command.ecc is not None else 0,                         # Fornecendo valor padrão
        delay=command.delay if command.delay is not None else 0,                    # Fornecendo valor padrão
        offset=command.offset if command.offset is not None else 0                  # Fornecendo valor padrão
    )

    send_date = datetime.now()

    result = operacao.send_command(cmd_type=nascerr_cmd.cmd_type,
                                   data_lenght=nascerr_cmd.data_lenght,
                                   id=nascerr_cmd.id,
                                   repeat=nascerr_cmd.repeat,
                                   write_type=nascerr_cmd.write_type,
                                   data=nascerr_cmd.data,
                                   mode=nascerr_cmd.mode,
                                   delay=nascerr_cmd.delay,
                                   offset=nascerr_cmd.offset
                                   )

    if result["error"]:
        status = 'FAIL'
    else:
        status = 'SUCCESS'

    # DEPOIS ADICIONAR BANCO DE DADOS
    # print(CommandEntry(command=nascerr_cmd,
    #                     send_date=send_date, status=status))
    
    command_list.append(CommandEntry(command=nascerr_cmd,
                        send_date=send_date, status=status))
    
    
    db.save_command_json(command_list)

    return RedirectResponse(url="/", status_code=303)

@app.get("/baixar_csv/{experiment_id}")
async def baixar_csv(experiment_id: str):
    # Busca o resultado correspondente ao experiment_id
    experiment_data = next(
    (
        entry for entry in operation_results 
        if entry.status != 'failed' and 'data' in entry.response and entry.response['data']['id'] == int(experiment_id)
    ), 
    None
    )

    if experiment_data:
        # Cria um arquivo CSV para salvar os dados do experimento
        matching_command = next(
            (entry for entry in command_list if entry.command.id == int(experiment_id)),
            None
        )

        if not matching_command:
            return {"error": "Comando correspondente não encontrado."}
        
        file_path = f"experimento_{experiment_id}.csv"
        saver = ExperimentResultSaver(file_path)
        # Salva os dados no CSV
        # print(experiment_data.response['data'])
        saver.save_result({
            "timestamp": experiment_data.receive_date,
            "id": experiment_data.response['data']['id'],
            "data_length": experiment_data.response['data']['data_length'],
            "number_errors": experiment_data.response['data']['number_errors'],
            "corrected_data": experiment_data.response['data']['corrected_data'],
            "percent_error": experiment_data.response['data']['percent_error'],
            "write_type": matching_command.command.write_type.name,
            "mode": matching_command.command.mode.name,
            "offset": matching_command.command.offset,
            "result_array": experiment_data.response['data']['result_array']
        })

        def remove_file():
            try:
                os.remove(file_path)
                print(f"Arquivo {file_path} excluído com sucesso.")
            except Exception as e:
                print(f"Erro ao excluir arquivo {file_path}: {str(e)}")

        # Retorna o arquivo CSV como resposta para download
        return FileResponse(file_path, media_type='text/csv', filename=file_path, background=BackgroundTask(remove_file))
    
    return {"error": "Experimento não encontrado"}

@app.post("/start_continuous")
async def run_batch_experiment(background_tasks: BackgroundTasks,request: Request,
    command: CommandFormModel = Depends(CommandFormModel.as_form)):
    async def continuous_task(command: CommandFormModel):
        global stop_requested
        stop_requested = False

        num_tests = command.repeat
        base_delay = command.delay

        ecc_options = list(NASCERR_MODE)
        write_type_options = list(W_DataTypeDef)    

        continuous_status["running"] = True
        continuous_status["total"] = num_tests
        continuous_status["current"] = 0
        next_offset = 0

        for i in range(num_tests): #todo: utilizar defines e definir limites corretos
            ecc_mode = command.ecc if command.ecc is not None else random.choice(ecc_options)
            data_len = command.data_length if command.data_length is not None else 1024
            write_type = command.write_type if command.write_type is not None else random.choice(write_type_options)
            read_value = command.read if command.read is not None else 1

            if command.offset is not None:
                offset = command.offset
            else:
                if next_offset + data_len > (MEM_LIM - 1):
                    next_offset = 0
                    print("atingiu o máximo e foi para zero")
                offset = next_offset  
                next_offset += data_len       

            if stop_requested:
                print("Parando execução contínua por solicitação do usuário.")
                break  # Sai do loop e encerra a execução

            nascerr_cmd = NASCERR_CTRL(
                cmd_type=CmdTypeDef.CONTINUOUS_CMD,
                id=id_gen.generate_id(),
                data_lenght=data_len,
                repeat=1,
                write_type=write_type,
                data=read_value,
                mode=ecc_mode,
                delay=base_delay,
                offset=offset
            )

            result = operacao.send_command(
                cmd_type=nascerr_cmd.cmd_type,
                data_lenght=nascerr_cmd.data_lenght,
                id=nascerr_cmd.id,
                repeat=nascerr_cmd.repeat,
                write_type=nascerr_cmd.write_type,
                data=nascerr_cmd.data,
                mode=nascerr_cmd.mode,
                delay=nascerr_cmd.delay,
                offset=nascerr_cmd.offset
            )  

            if result["error"]:
                status_res = 'FAIL'
            else:
                status_res = 'SUCCESS'

            start_time = datetime.now()
            command_list.append(CommandEntry(command=nascerr_cmd,
                        send_date=start_time, status=status_res))
            
            db.save_command_json(command_list)

            continuous_status["current"] = i + 1
            timeout = (nascerr_cmd.delay*60) + 30 # timeout da realização do experiemento em ms

            while True:
                await asyncio.sleep(1)
                status = operacao.get_status()

                if not status["error"]:
                    op = status["data"]["operation"]["name"]
                    if op in ["OP_TRANSMIT_RESPONSE", "OP_TRANSMIT_FINISH"]:
                        res = operacao.read_result_byPackages()
                        operation_results.append(ResponseEntry(
                            response=res,
                            receive_date=datetime.now(),
                            status='SUCCESS' if not res["error"] else 'FAIL'
                        ))
                        break

                # Verifica timeout
                elapsed = (datetime.now() - start_time).total_seconds()
                if elapsed > timeout:
                    operation_results.append(ResponseEntry(
                        response={"data": "Timeout"},
                        receive_date=datetime.now(),
                        status='TIMEOUT'
                    ))
                    break
            db.save_results_json(operation_results)
        continuous_status["running"] = False

    background_tasks.add_task(continuous_task, command)

    return RedirectResponse(url="/", status_code=303)

@app.get("/commands")
async def get_command_list():
    return JSONResponse(content=[
        {
            "id": cmd.command.id,
            "send_date": cmd.send_date.strftime('%Y-%m-%d %H:%M:%S'),
            "status": cmd.status
        } for cmd in command_list
    ])

@app.post("/stop_experiment")
async def stop_continuous_experiment():
    global stop_requested
    stop_requested = True
    return {"stopped": True}

if __name__ == "__main__":
    import uvicorn
    # Iniciar o servidor FastAPI
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
