import json
from enum import Enum
import struct


class OPERATION_EXP(Enum):
    """
    Enum for Operating Logs.
    """
    OP_EXP_OK = 0xA1
    OP_EXP_BUSY = 0xA2
    OP_EXP_RUNNING = 0xA3
    OP_EXP_RECEIVE_COMMAND = 0xA4
    OP_EXP_TRANSMIT_RESPONSE = 0xA5
    OP_EXP_TRANSMIT_FINISH = 0xA6
    OP_EXP_FINISH = 0xA7
    OP_EXP_FAIL = 0xA8

    @property
    def description(self):
        descriptions = {
            OPERATION_EXP.OP_EXP_OK: "Experiment Operation OK.",
            OPERATION_EXP.OP_EXP_BUSY: "Experiment is busy.",
            OPERATION_EXP.OP_EXP_RUNNING: "Experiment is running.",
            OPERATION_EXP.OP_EXP_RECEIVE_COMMAND: "Command received.",
            OPERATION_EXP.OP_EXP_TRANSMIT_RESPONSE: "Telemetry data transmitted.",
            OPERATION_EXP.OP_EXP_TRANSMIT_FINISH: "Telemetry data transmission finished.",
            OPERATION_EXP.OP_EXP_FINISH: "Operation has finished.",
            OPERATION_EXP.OP_EXP_FAIL: "Operation failed.",
        }
        return descriptions.get(self, "Unknown status")


class STRUCTURE_OPERATION_EXP:

    def __init__(self, id: int = 0, page: int = 0, stat: OPERATION_EXP = OPERATION_EXP.OP_EXP_OK):
        self.id = id
        self.page = page
        self.stat = stat

    def __repr__(self):
        return (
            f"STRUCTURE_OPERATION_EXP(id={self.id}, page={self.page}, "
            f"stat={self.stat.name}"
        )

    def to_dict(self):
        return {
            "id": self.id,
            "page": self.page,
            "stat": self.stat.value,
        }

    @property
    def description(self):
        descriptions = {
            STRUCTURE_OPERATION_EXP.id: "ID of the operation.",
            STRUCTURE_OPERATION_EXP.page: "Page of the operation.",
            STRUCTURE_OPERATION_EXP.stat: "Status of the operation."
        }
        return descriptions.get(self, "Unknown status")


class REQUEST_EXP(Enum):
    """
    Enum for Request Logs.
    """
    REQ_EXP_NOREQUEST = 0xA1
    REQ_EXP_TXRESULT = 0xA2

    @property
    def description(self):
        descriptions = {
            REQUEST_EXP.REQ_EXP_NOREQUEST: "No request has been made.",
            REQUEST_EXP.REQ_EXP_TXRESULT: "Transmission result request.",
        }
        return descriptions.get(self, "Unknown request")


# Definição dos possíveis tipos de operação e requisição de hardware
class OPERATION_HW(Enum):
    """
    Enum for Operating Logs.
    """
    OP_HW_OK = 0xB1
    OP_HW_SLEEP = 0xB2
    OP_HW_RECEIVED_COMMAND = 0xB3
    OP_HW_TRANSMIT_RESPONSE = 0xB4
    OP_HW_TRANSMIT_FINISH = 0xB5

    @property
    def description(self):
        descriptions = {
            OPERATION_HW.OP_HW_OK: "System is awake",
            OPERATION_HW.OP_HW_SLEEP: "System is sleeping",
            OPERATION_HW.OP_HW_RECEIVED_COMMAND: "Command received",
            OPERATION_HW.OP_HW_TRANSMIT_RESPONSE: "Hardware data transmitted",
            OPERATION_HW.OP_HW_TRANSMIT_FINISH: "Hardware data transmission finished",
        }
        return descriptions.get(self, "Unknown status")


class REQUEST_HW(Enum):
    """
    Enum for Request Logs.
    """

    REQ_HW_NOREQUEST = 0xB1
    REQ_HW_TRANSMIT_TEMPERATURE = 0xB2
    REQ_HW_TRANSMIT_INDIVIDUAL_CURRENTS = 0xB3
    REQ_HW_TRANSMIT_INDIVIDUAL_POWER = 0xB4
    REQ_HW_TRANSMIT_TOTAL_POWER = 0xB5

    @property
    def description(self):
        descriptions = {
            REQUEST_HW.REQ_HW_NOREQUEST: "No request has been made.",
            REQUEST_HW.REQ_HW_TRANSMIT_TEMPERATURE: "Request Temperature",
            REQUEST_HW.REQ_HW_TRANSMIT_INDIVIDUAL_CURRENTS: "Request Individual Currents",
            REQUEST_HW.REQ_HW_TRANSMIT_INDIVIDUAL_POWER: "Request Individual Power",
            REQUEST_HW.REQ_HW_TRANSMIT_TOTAL_POWER: "Request Total Power"
        }
        return descriptions.get(self, "Unknown request")


class NASCERR_STATUS:
    """
    Class representing NASCERR Status
    """

    def __init__(self,
                 structure_exp: STRUCTURE_OPERATION_EXP,
                 op_hw: OPERATION_HW,
                 req_exp: REQUEST_EXP,
                 req_hw: REQUEST_HW):
        self.structure_exp = structure_exp
        self.op_hw = op_hw
        self.req_exp = req_exp
        self.req_hw = req_hw

    def __repr__(self):
        return (
            f"NASCERR_STATUS(\n"
            f"  structure_exp={self.structure_exp},\n"
            f"  op_hw={self.op_hw.name},\n"
            f"  req_exp={self.req_exp.name},\n"
            f"  req_hw={self.req_hw.name},\n"
        )

    def to_dict(self):
        return {
            "operation": {
                "STRUCTURE_EXP": {
                    "id_value": self.structure_exp.id,
                    "page_value": self.structure_exp.page,
                    "stat_name": self.structure_exp.stat.name,
                    "stat_value": self.structure_exp.stat.value,
                    "stat_description": self.structure_exp.stat.description,
                },
                "HW": {
                    "name": self.op_hw.name,
                    "value": self.op_hw.value,
                    "description": self.op_hw.description
                }
            },
            "request": {
                "EXP": {
                    "name": self.req_exp.name,
                    "value": self.req_exp.value,
                    "description": self.req_exp.description
                },
                "HW": {
                    "name": self.req_hw.name,
                    "value": self.req_hw.value,
                    "description": self.req_hw.description
                }
            }
        }

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4)

    @classmethod
    def from_list(cls, lst):
        """
        Constroi uma instancia de NASCERR_STATUS a partir de uma lista de 6 valores:
        [op_hw, op_exp, op_log, req_hw, req_exp, req_log]

        @param lst: Lista contendo exatamente 6 inteiros.
        @return: Instância de NASCERR_STATUS.
        """
        if len(lst) != 9:
            raise ValueError(
                "the list must have 9 elements: [id_val_1, id_val_2, id_val_3, id_val_4, block_val, stat_val, req_exp_val, op_hw_val, req_hw_val]")

        id_val_1, id_val_2, id_val_3, id_val_4, block_val, stat_val, req_exp_val, op_hw_val, req_hw_val = lst

        try:
            obj = STRUCTURE_OPERATION_EXP(
                id=(id_val_1 << 8*3 | id_val_2 <<
                    8*2 | id_val_3 << 8 | id_val_4),
                page=block_val,
                stat=OPERATION_EXP(stat_val),
            )
        except ValueError:
            raise ValueError(f"Invalid value for OPERATION_EXP: {stat_val}")

        try:
            req_exp = REQUEST_EXP(req_exp_val)
        except ValueError:
            raise ValueError(f"Invalid value for REQUEST_EXP: {req_exp_val}")

        try:
            op_hw = OPERATION_HW(op_hw_val)
        except ValueError:
            raise ValueError(f"Invalid value for OPERATION_HW: {op_hw_val}")

        try:
            req_hw = REQUEST_HW(req_hw_val)
        except ValueError:
            raise ValueError(f"Invalid value for REQUEST_HW: {req_hw_val}")

        return cls(obj, req_exp, op_hw, req_hw)

# Adicionado novo status


class LOG_TYPE(Enum):
    """
    Enum for Type Logs.
    """
    LOG_VOID = 0x00
    LOG_SUCCESS = 0x01
    LOG_RUNNING_EXPERIMENT = 0x02
    LOG_RECEIVE_COMMAND = 0x03
    LOG_RECEIVE_HW_STAT_REQ = 0x04
    LOG_TRANSMIT_HW_RESPONSE = 0x05
    LOG_TRANSMIT_HW_FINISH = 0x06
    LOG_HW_WARN = 0x07
    LOG_HW_ERROR = 0x08
    LOG_TRANSMIT_RESPONSE = 0x09
    LOG_TRANSMIT_FINISH = 0x0A
    LOG_EXP_EXECUT = 0x0B
    LOG_EXP_FINNISH = 0x0C
    LOG_SYSTEM_BUSY = 0x0D
    LOG_FINISH = 0x0E
    LOG_FAIL = 0x0F
    LOG_MAX = 0x10
    LOG_BOOT_INIRIALIZED = 0x11
    LOG_BOOT_RECEIVE_COMMAND = 0x12
    LOG_BOOT_OPERATION = 0x13

    @property
    def description(self):
        descriptions = {
            LOG_TYPE.LOG_VOID:                "Sem log",
            LOG_TYPE.LOG_SUCCESS:             "Operacao bem-sucedida.",
            LOG_TYPE.LOG_RUNNING_EXPERIMENT:  "Experimento em execucao.",
            LOG_TYPE.LOG_RECEIVE_COMMAND:     "Comando recebido e em processamento.",
            LOG_TYPE.LOG_RECEIVE_HW_STAT_REQ: "Comando de recebibo de algum aspecto do hardware.",
            LOG_TYPE.LOG_TRANSMIT_HW_RESPONSE: "Resposta de hardware pronto para ser transmitida.",
            LOG_TYPE.LOG_TRANSMIT_HW_FINISH:  "Comando de hardware transmitido com sucesso.",
            LOG_TYPE.LOG_HW_WARN:             "Alerta de hardware.",
            LOG_TYPE.LOG_HW_ERROR:            "Erro de hardware.",
            LOG_TYPE.LOG_TRANSMIT_RESPONSE:   "Resposta em transmissao",
            LOG_TYPE.LOG_TRANSMIT_FINISH:     "Transmissão finalizada com sucesso",
            LOG_TYPE.LOG_EXP_EXECUT:          "Experimento em execucao",
            LOG_TYPE.LOG_EXP_FINNISH:         "Experimento finalizado com sucesso",
            LOG_TYPE.LOG_SYSTEM_BUSY:         "Sistema ocupado, nao pode aceitar novo comando",
            LOG_TYPE.LOG_FINISH:              "Operacao finalizada (status generico)",
            LOG_TYPE.LOG_FAIL:                "Operacao falhou (erro generico)",
            LOG_TYPE.LOG_MAX:                 "Valor maximo",
            LOG_TYPE.LOG_BOOT_INIRIALIZED:    "Bootloader inicializado",
            LOG_TYPE.LOG_BOOT_RECEIVE_COMMAND: "Bootloader recebeu comando",
            LOG_TYPE.LOG_BOOT_OPERATION:      "Operacao do bootloader em andamento",
        }
        return descriptions.get(self, "Unknown request")


class ERROS_LOGS(Enum):
    """
    Enum for Error Logs.
    """
    ERR_VOID = 0x00
    ERR_NONE = 0x01
    WARN_COMM_DECODE = 0x02
    WARM_HW_STATUS_CODE = 0x03

    ERR_COMM_SENDPKT = 0x04
    ERR_EXP_INVALID_DATALEN = 0x05
    ERR_EXP_MEM_OVERFLOW = 0x06
    ERR_EXP_INVALID_REPETITIONS = 0x07
    ERR_EXP_INVALID_DELAY = 0x08
    ERR_I2C_INIT_FAILED = 0x09
    ERR_I2C_BUSY = 0x0A
    ERR_I2C_TRANSFER = 0x0B
    ERR_I2C_TIMEOUT = 0x0C
    ERR_FLASH_OPERATION = 0x0D
    ERR_FLASH_ERASE_FAILED = 0x0E
    ERR_PROTO_CMD_DECODE = 0x0F
    ERR_PROTO_SEND_PACKET = 0x10
    ERR_COMM_GENERAL = 0x11
    ERR_CMD_INVALID = 0x12

    ERR_BOOT_CMD_SIZE = 0x1A
    ERR_BOOT_CMD_INIT = 0x1B
    ERR_BOOT_CMD_DECODE = 0x1C
    ERR_BOOT_FLASH_WRITE = 0x1D
    ERR_BOOT_FLASH_ERASE = 0x1E
    ERR_BOOT_FLASH_WRITE_SIZE = 0x1F
    ERR_BOOT_FLASH_CRC = 0x20

    ERR_BOOT_PKG_OUT_OF_SEQ = 0x22
    ERR_BOOT_PKG_INVALID_VER = 0x23
    ERR_BOOT_TIMEOUT = 0x24

    WARN_HW_CURRENT_THRESHOLD = 0x25

    ERR_BOOT_INVALID_METADATA = 0x26
    ERR_BOOT_SHA_MISMATCH = 0x27
    ERR_BOOT_UNRECOVERABLE = 0x28

    @property
    def description(self):
        descriptions = {
            ERROS_LOGS.ERR_VOID: "Sem erro",
            ERROS_LOGS.ERR_NONE: "Sem erro",
            ERROS_LOGS.WARN_COMM_DECODE: "Falha na decodificaco do comando",
            ERROS_LOGS.WARM_HW_STATUS_CODE: "Tentativa de acesso de status de hardware inexistente",
            ERROS_LOGS.ERR_COMM_SENDPKT: "Falha no envio do pacote",
            ERROS_LOGS.ERR_EXP_INVALID_DATALEN: "Experimento interrompido: dado de tamanho invalido",
            ERROS_LOGS.ERR_EXP_MEM_OVERFLOW: "Experimento interrompido: estouro de região de memoria",
            ERROS_LOGS.ERR_EXP_INVALID_REPETITIONS: "Experimento interrompido: numero de repeticoes invalido",
            ERROS_LOGS.ERR_EXP_INVALID_DELAY: "Experimento interrompido: valor de delay invalido",
            ERROS_LOGS.ERR_I2C_INIT_FAILED: "Erro na inicialização da comunicacao I2C",
            ERROS_LOGS.ERR_I2C_BUSY: "Comunicação I2C ocupada",
            ERROS_LOGS.ERR_I2C_TRANSFER: "Erro na transmissao/recepção I2C",
            ERROS_LOGS.ERR_I2C_TIMEOUT: "Transmissao finalizada por atingir timeout",
            ERROS_LOGS.ERR_FLASH_OPERATION: "Erro de operacao na flash",
            ERROS_LOGS.ERR_FLASH_ERASE_FAILED: "Erro ao apagar bloco na flash",
            ERROS_LOGS.ERR_PROTO_CMD_DECODE: "Falha na decodificacao do comando",
            ERROS_LOGS.ERR_PROTO_SEND_PACKET: "Falha no envio do pacote",
            ERROS_LOGS.ERR_COMM_GENERAL: "Erro de comunicacao generico",
            ERROS_LOGS.ERR_CMD_INVALID: "Comando invalido ou desconhecido ",
            ERROS_LOGS.ERR_BOOT_CMD_SIZE: "Tamanho do comando do bootloader invalido",
            ERROS_LOGS.ERR_BOOT_CMD_INIT: "Erro ao iniciar o comando do bootloader",
            ERROS_LOGS.ERR_BOOT_CMD_DECODE: "Erro ao decodificar o comando do bootloader",
            ERROS_LOGS.ERR_BOOT_FLASH_WRITE: "Erro ao escrever na flash",
            ERROS_LOGS.ERR_BOOT_FLASH_ERASE: "Erro ao apagar a flash",
            ERROS_LOGS.ERR_BOOT_FLASH_WRITE_SIZE: "Tamanho da escrita na flash invalido",
            ERROS_LOGS.ERR_BOOT_FLASH_CRC: "Erro de CRC da flash",
            ERROS_LOGS.ERR_BOOT_PKG_OUT_OF_SEQ: "Pacote fora de sequencia",
            ERROS_LOGS.ERR_BOOT_PKG_INVALID_VER: "Versao do pacote invalida",
            ERROS_LOGS.ERR_BOOT_TIMEOUT: "Timeout do bootloader",
            ERROS_LOGS.WARN_HW_CURRENT_THRESHOLD: "Alerta de corrente acima do limite",
            ERROS_LOGS.ERR_BOOT_INVALID_METADATA: "Metadados do firmware invalidos",
            ERROS_LOGS.ERR_BOOT_SHA_MISMATCH: "SHA-256 do firmware nao confere",
            ERROS_LOGS.ERR_BOOT_UNRECOVERABLE: "Erro critico no bootloader",
        }
        return descriptions.get(self, "Unknown error")


class NASCERR_MODE(Enum):
    """
    Enum for NASCERR Mode.
    """
    BYPASS_1 = 1
    BYPASS_2 = 2
    TBEC = 3
    MRSC = 4
    DUAL_BYPASS = 5

    @property
    def description(self):
        descriptions = {
            NASCERR_MODE.BYPASS_1: "Bypass operation to SRAM 1.",
            NASCERR_MODE.BYPASS_2: "Bypass operation to SRAM 2.",
            NASCERR_MODE.TBEC: "Error Correction Code TBEC_RSC in both SRAMs.",
            NASCERR_MODE.MRSC: "Error Correction Code MRSC in both SRAMs.",
            NASCERR_MODE.DUAL_BYPASS: "Bypass in SRAM1 and SRAM2"
        }
        return descriptions.get(self, "Unknown mode")

    def to_dict(self):
        return {
            "mode": {
                "name": self.name,
                "value": hex(self.value),
                "description": self.description
            }
        }

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4)


class NASCERR_LOG:
    """
    Class representing NASCERR Logs.
    """

    def __init__(self, timestamp: int, type: LOG_TYPE, status: int, error_code: ERROS_LOGS):
        self.timestamp = timestamp
        self.type = type
        self.status = status
        self.error_code = error_code

    def __repr__(self):
        return (f"NASCERR_LOG(timestamp={self.timestamp}, "
                f"type={self.type.name}, "
                f"status={self.status}, "
                f"error_code={self.error_code.name})")

    def to_dict(self):
        return {
            "timestamp": self.timestamp,
            "type": {
                "name": self.type.name,
                "value": self.type.value,
                "description": self.type.description
            },
            "status": self.status,
            "error_code": {
                "name": self.error_code.name,
                "value": self.error_code.value,
                "description": self.error_code.description
            }
        }

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4)

    @classmethod
    def from_list(cls, lst):
        """
        Converte uma lista de 7 elementos em uma instância de NASCERR_LOG.
        """
        if len(lst) != 7:
            raise ValueError(
                "A lista deve conter exatamente 7 elementos (timestamp, type, status, error_code).")

        timestamp_value = int.from_bytes(lst[0:4], byteorder='little')
        type_value = lst[4]
        status_value = lst[5]
        error_code_value = lst[6]

        # Verifica se os valores são válidos
        if type_value not in LOG_TYPE._value2member_map_:
            raise ValueError(f"Valor invalido para LOG_TYPE: {type_value}")
        if error_code_value not in ERROS_LOGS._value2member_map_:
            raise ValueError(
                f"Valor invalido para ERROS_LOGS: {error_code_value}")

        return cls(timestamp_value, LOG_TYPE(type_value), status_value, ERROS_LOGS(error_code_value))

    @classmethod
    def parse_multiple(cls, byte_list):
        logs = []
        for i in range(0, len(byte_list), 7):
            chunk = byte_list[i:i+7]
            if len(chunk) < 7:
                break  # Ignora pedaço incompleto
            logs.append(cls.from_list(chunk))
        return logs


class NASCERR_EXP_STATUS:
    """
    Class representing NASCERR experiment status telemetry.
    """
    _FORMAT = "<7H2B"  # little-endian: 7×uint16 (H), 2×uint8 (B)
    SIZE = struct.calcsize(_FORMAT)

    def __init__(self,
                 qtd_scheduler_total: int,
                 qtd_scheduler_pending: int,
                 qtd_scheduler_executed: int,
                 qtd_experiments_total: int,
                 qtd_experiments_running: int,
                 qtd_experiments_finished: int,
                 qtd_experiments_unread: int,
                 scheduler_full_flag: int,
                 flash_full_flag: int):
        self.qtd_scheduler_total = qtd_scheduler_total
        self.qtd_scheduler_pending = qtd_scheduler_pending
        self.qtd_scheduler_executed = qtd_scheduler_executed

        self.qtd_experiments_total = qtd_experiments_total
        self.qtd_experiments_running = qtd_experiments_running
        self.qtd_experiments_finished = qtd_experiments_finished
        self.qtd_experiments_unread = qtd_experiments_unread

        self.scheduler_full_flag = scheduler_full_flag
        self.flash_full_flag = flash_full_flag

    @classmethod
    def from_bytes(cls, data: bytes) -> "NASCERR_EXP_STATUS":
        """
        Unpack the first SIZE bytes of `data` into a new instance.
        """
        if len(data) < cls.SIZE:
            raise ValueError(
                f"Data too short: {len(data)} bytes (need {cls.SIZE})")
        values = struct.unpack(cls._FORMAT, data[:cls.SIZE])
        return cls(*values)

    def to_bytes(self) -> bytes:
        """
        Pack all fields into a byte buffer ready to send over I2C.
        """
        return struct.pack(
            self._FORMAT,
            self.qtd_scheduler_total,
            self.qtd_scheduler_pending,
            self.qtd_scheduler_executed,
            self.qtd_experiments_total,
            self.qtd_experiments_running,
            self.qtd_experiments_finished,
            self.qtd_experiments_unread,
            self.scheduler_full_flag,
            self.flash_full_flag
        )

    def to_dict(self) -> dict:
        return {
            "qtd_scheduler_total":    self.qtd_scheduler_total,
            "qtd_scheduler_pending":  self.qtd_scheduler_pending,
            "qtd_scheduler_executed": self.qtd_scheduler_executed,
            "qtd_experiments_total":    self.qtd_experiments_total,
            "qtd_experiments_running":  self.qtd_experiments_running,
            "qtd_experiments_finished": self.qtd_experiments_finished,
            "qtd_experiments_unread":   self.qtd_experiments_unread,
            "scheduler_full_flag": self.scheduler_full_flag,
            "flash_full_flag":     self.flash_full_flag
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=4)

    def __repr__(self) -> str:
        d = self.to_dict()
        fields = ", ".join(f"{k}={v}" for k, v in d.items())
        return f"NASCERR_EXP_STATUS({fields})"


class FlashRegionTypeDef(Enum):
    """
    Enum que espelha o typedef FlashRegionTypeDef do firmware.

    value → índice usado pelo firmware
    """
    FLASH_HEADER = 0
    FLASH_LOGS = 1
    FLASH_SCHEDULER = 2
    FLASH_EXPERIMENT = 3
    FLASH_BACKUP_FIRMWARE = 4
    MAX_FLASH_REGION = 5

    @property
    def description(self) -> str:
        desc = {
            FlashRegionTypeDef.FLASH_HEADER:          "Região de header/meta-dados",
            FlashRegionTypeDef.FLASH_LOGS:            "Área de logs de sistema",
            FlashRegionTypeDef.FLASH_SCHEDULER:       "Área de comandos agendados",
            FlashRegionTypeDef.FLASH_EXPERIMENT:      "Área de experimentos gravados",
            FlashRegionTypeDef.FLASH_BACKUP_FIRMWARE: "Backup de firmware/boot",
            FlashRegionTypeDef.MAX_FLASH_REGION:      "Valor sentinela (não usar)",
        }
        return desc.get(self, "Unknown region")

    # helpers opcionais
    def to_dict(self):
        return {
            "name":        self.name,
            "value":       self.value,
            "description": self.description,
        }

    def __repr__(self):
        return f"FlashRegionTypeDef.{self.name}({self.value})"
