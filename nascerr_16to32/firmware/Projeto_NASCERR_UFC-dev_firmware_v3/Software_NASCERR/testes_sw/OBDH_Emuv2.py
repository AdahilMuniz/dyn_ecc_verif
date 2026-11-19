import json
from time import sleep
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_protocol import Protocol
from nascerr_modules.nascerr_operation import Operation
from nascerr_modules.nascerr_image_processing import *

# Inicializa protocolo e operacao
device_name = "NASCERR_CargaUtil"
i2c_address = 0x55
operacao = Operation(device_name, i2c_address)
protocol = Protocol("Nascerr", i2c_address)

# Mostra status atual do dispositivo
status = operacao.get_status()
print(json.dumps(status, indent=4))
sleep(2)

print("########## ENVIO DE 64 COMANDOS ##########")

for blk in range(1, 10):                       # block sequencial 1..64
    resultado = operacao.send_commandNew(
        cmd_type=CmdTypeDef.WRITE_CMD,
        id=blk,                                # id igual ao bloco (ajuste se desejar)
        block=blk,
        qtd_block=1,                           # sempre 1
        ecc=NASCERR_MODE.MRSC,
        write_type=W_DataTypeDef.WRITE_xAAh,
        scheduler_time=10000,                  # 10 s
        repeat=1,
        delay=10000,                         # delay igual ao scheduler_time
        current_tick=0
    )
    print(f"Cmd #{blk:02d} ->", json.dumps(resultado, indent=4))

    sleep(0.1)

# Reinicia barramento I2C ao final
protocol.i2c.restart_i2c_bus()