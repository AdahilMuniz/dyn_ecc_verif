import struct
from time import sleep
from nascerr_modules.nascerr_command import *
from nascerr_modules.nascerr_response import *
from nascerr_modules.nascerr_system import *
from nascerr_modules.nascerr_hardware import *
from nascerr_modules.nascerr_registers import registers, AccessType
from nascerr_modules.util.crc import calculate_crc16, verify_crc16
from nascerr_modules.util.i2c_interface import I2CInterface
from nascerr_modules.nascerr_bootloader import NASCERR_BOOT, NASCERR_STATUS_BOOT, BootCommand
from nascerr_modules.nascerr_gps import NASCERR_GPS

count: int = 0
list_count: list[int] = []


class Protocol:
    def __init__(self, device_name, i2c_address):
        self.device_name = device_name
        self.i2c = I2CInterface(addr=i2c_address)

    def set_register(self, register_name, data):
        register = registers.get(register_name)

        if not register:
            raise ValueError(f"Register '{register_name}' does not exist.")

        if register.access not in [AccessType.WRITE_ONLY, AccessType.FULL_ACCESS]:
            raise PermissionError(
                f"Write access denied for register '{register_name}'.")

        if len(data) > register.size:
            raise ValueError(
                f"Data length exceeds the maximum size of {register.size} bytes for register '{register_name}'.")

        result = self.i2c.write_data(register.addr, data)

        return result

    def get_register(self, register_name):
        register = registers.get(register_name)

        if not register:
            raise ValueError(f"Register '{register_name}' does not exist.")

        if register.access not in [AccessType.READ_ONLY, AccessType.FULL_ACCESS]:
            raise PermissionError(
                f"Read access denied for register '{register_name}'.")

        data = self.i2c.read_data(register.addr, register.size)

        return data

    def get_register_withsize(self, register_name, size):
        register = registers.get(register_name)

        if not register:
            raise ValueError(f"Register '{register_name}' does not exist.")

        if register.access not in [AccessType.READ_ONLY, AccessType.FULL_ACCESS]:
            raise PermissionError(
                f"Read access denied for register '{register_name}'.")

        # Usando o 'size' fornecido em vez de 'register.size'
        data = self.i2c.read_data(register.addr, size)
        return data

    def get_status(self):
        status_register_name = "STAT"
        data_status = self.get_register(status_register_name)

        if isinstance(data_status["data"], list):
            status_obj = NASCERR_STATUS.from_list(data_status["data"][:-2])
            hex_data = ''.join(format(x, '02X') for x in data_status["data"])

            verify_crc = verify_crc16(hex_data)
            if not verify_crc:
                raise ValueError("Invalid CRC in STAT register data.")
            else:
                # Remover os dois últimos bytes (CRC) dos dados antes de processá-los
                data_status["data"] = data_status["data"][:-2]
            status_obj = NASCERR_STATUS.from_list(data_status["data"])
        else:
            raise ValueError(
                "Data read from register is not in expected format.")

        return {
            "error": data_status["error"],
            "message": data_status["message"],
            "data": status_obj.to_dict()
        }

    def get_log(self):
        log_register_name = "LOG"
        data_log = self.get_register(log_register_name)

        if not isinstance(data_log["data"], list):
            raise ValueError(
                "Data read from register is not in expected format.")

        full_data = data_log["data"]

        hex_data = ''.join(format(x, '02X') for x in full_data)
        # Verifica CRC com todos os dados
        if not verify_crc16(hex_data):
            raise ValueError("Invalid CRC in LOG register data.")
        else:
            # Remove os dois últimos bytes (CRC) dos dados antes de processá-los
            log_bytes = full_data[:-2]

            if len(log_bytes) < 7:
                raise ValueError("Insufficient data for LOG register.")

            # log_obj = NASCERR_LOG.from_list(log_bytes[:7])
            logs_obj = NASCERR_LOG.parse_multiple(log_bytes)

            # return {
            #     "timestamp": log_obj.timestamp,
            #     "type": log_obj.type.name,
            #     "status": log_obj.status,
            #     "error_code": log_obj.error_code.name,
            #     "data": log_obj.to_dict()
            # }
            return [
                {
                    "timestamp": log_obj.timestamp,
                    "type": log_obj.type.name,
                    "status": log_obj.status,
                    "error_code": log_obj.error_code.name,
                    "data": log_obj.to_dict()
                }
                for log_obj in logs_obj
            ]

    def get_mode(self):
        mode_register_name = "MODE"
        data_mode = self.get_register(mode_register_name)

        if isinstance(data_mode["data"], list) and len(data_mode["data"]) == 1:
            mode_value = data_mode["data"][0]
            try:
                mode_enum = NASCERR_MODE(mode_value)
                return {
                    "error": data_mode["error"],
                    "message": data_mode["message"],
                    "data": mode_enum.to_dict()
                }
            except ValueError:
                return {
                    "error": True,
                    "message": f"Invalid mode value read: {mode_value}",
                    "data": None
                }
        else:
            return {
                "error": True,
                "message": "Data read from mode register is not in expected format",
                "data": None
            }

    def set_command(self, cmd_type, id, data_lenght, repeat, write_type, data, mode, delay, offset, exp_image_ctrl=None):
        ctrl = NASCERR_CTRL(
            cmd_type=cmd_type,
            id=id,
            data_lenght=data_lenght,
            repeat=repeat,
            write_type=write_type,
            data=data,
            mode=mode,
            delay=delay,
            offset=offset
        )

        command_bytes_info = ctrl.to_bytes()

        command_bytes = command_bytes_info["message_bytes"]
        crc16 = command_bytes_info["crc16"]

        result = self.set_register("CMD", command_bytes)

        return {
            "error": result["error"],
            "message": result["message"],
            "command_bytes": command_bytes.hex(),
            "crc16": f"{crc16:04X}",
            "command_dict": ctrl.to_dict()
        }

    def get_response(self):
        """
        Get and parse the response from the device.
        """
        # Primeiro, obtenha o tamanho dos dados da resposta do registro "IDX"
        sleep(0.01)
        idx_register_name = "IDX"
        ack_register_name = "ACK"
        idx_data = self.get_register(idx_register_name)

        ack_fail = struct.pack('<H', 0)
        ack_fail += struct.pack('<H', calculate_crc16(ack_fail))

        if len(idx_data["data"]) != 4:
            print("erro em idx")
            raise ValueError(
                "Invalid data length for IDX register. Expected 4 bytes (2 bytes for size, 2 bytes for CRC).")

        hex_data_idx = ''.join(format(x, '02X') for x in idx_data["data"])
        verify_crc = verify_crc16(hex_data_idx)
        if not verify_crc:
            self.set_register(ack_register_name, ack_fail)
            raise ValueError("Invalid CRC in IDX register data.")
        else:
            response_data_length = struct.unpack(
                '<H', bytes(idx_data["data"][:2]))[0]

        txb_register_name = "TXB"
        txb_data = self.get_register_withsize(
            txb_register_name, response_data_length)

        if len(txb_data["data"]) != response_data_length:
            self.set_register(ack_register_name, ack_fail)
            raise ValueError("Response data length mismatch.")

        hex_data = ''.join(format(x, '02X') for x in txb_data["data"])
        verify_crc = verify_crc16(hex_data)

        # Verificar CRC da Resposta
        if not verify_crc:
            self.set_register(ack_register_name, ack_fail)
            # print(result_ack)
            raise ValueError("Invalid CRC.")

        else:
            response_data_length = response_data_length - 2
            txb_data["data"] = txb_data["data"][:-2]

        # Crie uma instância de NASCERR_RESPONSE a partir dos dados lidos
        try:
            # Desempacote os dados básicos
            header_data = txb_data["data"][:25]
            id, data_length, number_errors, corrected_data, percent_error, command, number_packages, package_number, error_by_package = struct.unpack(
                '<H I I I f B H H H',  bytes(header_data))

            message_ack = struct.pack('<H', package_number)
            message_ack += struct.pack('<H', calculate_crc16(message_ack))
            self.set_register(ack_register_name, message_ack)

            # Extraia os erros e dados corrigidos da resposta
            result_data = txb_data["data"][25:]
            result_array = []
            error_size = 8

            error_last_package = number_errors - \
                ((number_packages-1)*error_by_package)

            if (number_packages-package_number != 0) and (len(result_data) < error_by_package * error_size) and (CmdTypeDef(command) != CmdTypeDef.EXPIMAGE_CMD):
                raise ValueError("Insufficient data for errors.")
            if (number_packages-package_number == 0) and (len(result_data) < error_last_package * error_size) and (CmdTypeDef(command) != CmdTypeDef.EXPIMAGE_CMD):
                raise ValueError("Insufficient data for errors.")

            # if CmdTypeDef(command) != CmdTypeDef.EXPIMAGE_CMD:
            if (number_packages-package_number != 0):
                for i in range(error_by_package):
                    start = i * error_size
                    end = start + error_size
                    addr, result = struct.unpack(
                        '<I I', bytes(result_data[start:end]))
                    result_array.append(DescriptionError(addr, result))
            else:
                for i in range(error_last_package):
                    start = i * error_size
                    end = start + error_size
                    addr, result = struct.unpack(
                        '<I I', bytes(result_data[start:end]))
                    result_array.append(DescriptionError(addr, result))

            response_obj = NASCERR_RESPONSE(
                id=id,
                data_length=data_length,
                number_errors=number_errors,
                corrected_data=corrected_data,
                percent_error=percent_error,
                command=command,
                number_packages=number_packages,
                package_number=package_number,
                error_by_package=error_by_package,
                result_array=result_array
            )
            # print(response_obj.to_dict())
            return {
                "error": txb_data["error"],
                "message": txb_data["message"],
                "data": response_obj.to_dict()
            }

        except Exception as e:
            return {
                "error": True,
                "message": f"Data read from response register is not in expected format: {str(e)}",
                "data": None
            }

    def boot_get_status(self):
        """
        Requests and parses the BOOT_STAT register. Returns a dict:
          {
            "error":   bool,
            "message": str,
            "data":    { "version":…, "flags":…, "last_error":… }
          }
        """
        register_name = "BOOT_STAT"

        resp = self.get_register(register_name)
        if resp["error"]:
            # pass through any bus error
            return resp

        data = resp["data"]
        if not isinstance(data, list) or len(data) < 15:
            raise ValueError("BOOT_STAT: expected 15 bytes of data, got "
                             f"{len(data) if isinstance(data, list) else type(data)}")
        
        hex_data = ''.join(format(x, '02X') for x in data)
        verify_crc = verify_crc16(hex_data)
        if not verify_crc:
            raise ValueError("Invalid CRC in STAT register data.")

        raw_bytes = data[:13]
        boot_obj = NASCERR_STATUS_BOOT.from_list(raw_bytes)

        return {
            "error": resp["error"],
            "message": resp["message"],
            "data": boot_obj.to_dict()
        }

    def boot_init(self, firmware_size: int, version: int, num_packets: int):
        """ Initializes the bootloader for firmware download.
        This function sends a command to the device to prepare for firmware download.
        It sets the firmware size and version, and prepares the device for the download process
        """
        register_name = "BOOT_DATA"

        # Create a NASCERR_BOOT command for download initialization
        boot_ctrl = NASCERR_BOOT(
            cmd=BootCommand.DOWNLOAD_INIT
        )
        # Set the firmware size and version in the command
        boot_init_ctrl = boot_ctrl.make_download_init(
            fw_size=firmware_size, version=version, num_packets=num_packets)
        
        boot_init_bytes_info = boot_init_ctrl.to_bytes()
        boot_init_bytes = boot_init_bytes_info["message_bytes"]
        crc16 = boot_init_bytes_info["crc16"]

        result = self.set_register(register_name, boot_init_bytes)

        return {
            "error": result["error"],
            "message": result["message"],
            "command_bytes": boot_init_bytes.hex(),
            "crc16": f"{crc16:04X}",
            "command_dict": boot_init_ctrl.to_dict()
        }

    def boot_data_transfer(self, id_pkg: int, data: bytes):
        """ Transfers a chunk of data to the bootloader.
        This function sends a chunk of data to the device as part of the firmware download process.
        It is used to send the actual firmware data after the download initialization.
        """
        register_name = "BOOT_DATA"

        # Create a NASCERR_BOOT command for download initialization
        boot_ctrl = NASCERR_BOOT(
            cmd=BootCommand.DATA_TRANSFER
        )

        boot_init_ctrl = boot_ctrl.make_data_transfer(
            packet_id=id_pkg, chunk=data)
        boot_init_bytes_info = boot_init_ctrl.to_bytes()
        boot_init_bytes = boot_init_bytes_info["message_bytes"]
        crc16 = boot_init_bytes_info["crc16"]

        result = self.set_register(register_name, boot_init_bytes)

        return {
            "error": result["error"],
            "message": result["message"],
            "command_bytes": boot_init_bytes.hex(),
            "crc16": f"{crc16:04X}",
            "command_dict": boot_init_ctrl.to_dict()
        }
    
    def boot_donwload_finish(self, version: int, sha: bytes):
        """ Finalizes the bootloader download process.
        This function sends a command to the device to finalize the firmware download.
        It is used to complete the firmware update process after all data has been sent.
        """
        register_name = "BOOT_DATA"

        # Create a NASCERR_BOOT command for download initialization
        boot_ctrl = NASCERR_BOOT(
            cmd=BootCommand.DOWNLOAD_FINISH
        )

        boot_init_ctrl = boot_ctrl.make_download_finish(version=version, sha=sha)
        boot_init_bytes_info = boot_init_ctrl.to_bytes()
        boot_init_bytes = boot_init_bytes_info["message_bytes"]
        crc16 = boot_init_bytes_info["crc16"]

        result = self.set_register(register_name, boot_init_bytes)

        return {
            "error": result["error"],
            "message": result["message"],
            "command_bytes": boot_init_bytes.hex(),
            "crc16": f"{crc16:04X}",
            "command_dict": boot_init_ctrl.to_dict()
        }

    def set_commandNew(
            self,
            cmd_type,
            id,
            block,
            qtd_block,
            ecc,
            write_type,
            scheduler_time,
            repeat,
            delay,
            current_tick):
        """
        Envia um comando NASCERR_CMD para o registrador CMD.

        Parâmetros correspondem aos campos do NASCERR_CMD.
        """
        # Monta o comando
        cmd = NASCERR_CMD(
            cmd_type=cmd_type,
            id=id,
            block=block,
            qtd_block=qtd_block,
            ecc=ecc,
            write_type=write_type,
            scheduler_time=scheduler_time,
            repeat=repeat,
            delay=delay,
            current_tick=current_tick
        )

        # Serializa
        cmd_info = cmd.to_bytes()
        command_bytes = cmd_info["message_bytes"]
        crc16 = cmd_info["crc16"]

        # Grava no registrador “CMD”
        result = self.set_register("CMD", command_bytes)

        return {
            "error":         result["error"],
            "message":       result["message"],
            "command_bytes": command_bytes.hex(),
            "crc16":         f"{crc16:04X}",
            "command_dict":  cmd.to_dict()
        }

    def get_experiment_status(self):
        register = "EXP_STAT"
        resp = self.get_register(register)

        if not isinstance(resp["data"], list):
            raise ValueError(
                "Data read from register is not in expected format.")

        # monta string hex para CRC
        hex_str = "".join(f"{b:02X}" for b in resp["data"])
        if not verify_crc16(hex_str):
            raise ValueError(f"Invalid CRC in {register} register data.")

        # remove os 2 bytes de CRC e converte em bytes
        payload = bytes(resp["data"][:-2])
        exp_status = NASCERR_EXP_STATUS.from_bytes(payload)

        return {
            "error":   resp.get("error"),
            "message": resp.get("message"),
            "data":    exp_status.to_dict()
        }

    def set_expSumIDX(self, offset: int) -> dict:
        """
        Write <offset> to the EXP_SUM_IDX register.

        • offset precisa ser múltiplo de 30 (tamanho de página)  
        • offset ≤ 255 (um único byte, segundo o firmware)

        Retorna o dicionário padrão da sua `set_register`.
        """
        register = "EXP_SUM_IDX"

        if offset % 30 != 0:
            raise ValueError("offset deve ser múltiplo de 30 (0, 30, 60 …)")
        if not 0 <= offset <= 0xFF:
            raise ValueError("offset deve caber em 1 byte (0-255)")

        message = struct.pack('<B', offset)

        crc16 = calculate_crc16(message)
        message += struct.pack('>H', crc16)

        result = self.set_register(register, message)

        return {
            "error":   result["error"],
            "message": result["message"],
            "data":    {
                "offset": offset,
                "crc16":  f"{crc16:04X}"
            }
        }

    def get_expSUM(self) -> dict:
        """
        Read EXP_SUM_DATA, valida CRC16 e devolve objeto NASCERR_EXP_SUMMARY.

        Use depois de chamar `set_expSumIDX()`.
        """
        register = "EXP_SUM_DATA"
        resp = self.get_register(register)
        # print("resp: ", resp)
        if not isinstance(resp["data"], list):
            raise ValueError(
                "Dados de EXP_SUM_DATA não estão no formato esperado")

        # CRC16: ultimo par de bytes
        hex_str = "".join(f"{b:02X}" for b in resp["data"])
        if not verify_crc16(hex_str):
            raise ValueError(f"CRC inválido em {register}")

        # Remove CRC (2 bytes finais) e faz parse
        payload = bytes(resp["data"][:-2])
        summary = NASCERR_EXP_SUMMARY.from_bytes(payload)

        return {
            "error":   resp.get("error"),
            "message": resp.get("message"),
            "data":    summary.to_dict(),
        }

    def get_temperature(self):
        """
        Le o registrador HW_STAT e retorna os valores de temperature
        via HW_TXB.
        """
        hardware = NASCERR_HARDWARE("temperature")
        hw_stat_register_name = "HW_STAT"
        payload = hardware.write_hw_status_payload()
        write = self.set_register(hw_stat_register_name, payload["data"])

        if write["error"]:
            return {
                "error": True,
                "message": write["message"],
                "data": None
            }
        sleep(0.1)

        hw_stat_register_name = "HW_TXB"
        buff_size = hardware.get_size_buff_tx()
        result = self.get_register_withsize(hw_stat_register_name, buff_size)
        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return {
            "error": False,
            "message": result["message"],
            "data": hardware.read_decode_hw_status(result)
        }

    def get_total_current(self):
        """
       Le o registrador HW_STAT e retorna os valores de total_current via HW_TXB.
        """
        hardware = NASCERR_HARDWARE("current")
        hw_stat_register_name = "HW_STAT"
        payload = hardware.write_hw_status_payload()
        write = self.set_register(hw_stat_register_name, payload["data"])

        if write["error"]:
            return {
                "error": True,
                "message": write["message"],
                "data": None
            }
        sleep(0.1)

        hw_stat_register_name = "HW_TXB"
        buff_size = hardware.get_size_buff_tx()
        result = self.get_register_withsize(hw_stat_register_name, buff_size)
        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        valor = sum(hardware.read_decode_hw_status(result))
        return {
            "error": False,
            "message": result["message"],
            "data": [valor]
        }

    def get_current(self):
        """
        Le o registrador HW_STAT e retorna os valores de current via HW_TXB.
        """
        hardware = NASCERR_HARDWARE("current")
        hw_stat_register_name = "HW_STAT"
        payload = hardware.write_hw_status_payload()
        write = self.set_register(hw_stat_register_name, payload["data"])

        if write["error"]:
            return {
                "error": True,
                "message": write["message"],
                "data": None
            }
        sleep(0.1)

        hw_stat_register_name = "HW_TXB"
        buff_size = hardware.get_size_buff_tx()
        result = self.get_register_withsize(hw_stat_register_name, buff_size)
        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return {
            "error": False,
            "message": result["message"],
            "data": hardware.read_decode_hw_status(result)
        }

    def get_total_power(self):
        """
        Le o registrador HW_STAT e retorna os valores de power total
        via HW_TXB.
        """
        hardware = NASCERR_HARDWARE("total_power")
        hw_stat_register_name = "HW_STAT"
        payload = hardware.write_hw_status_payload()
        write = self.set_register(hw_stat_register_name, payload["data"])

        if write["error"]:
            return {
                "error": True,
                "message": write["message"],
                "data": None
            }
        sleep(0.1)

        hw_stat_register_name = "HW_TXB"
        buff_size = hardware.get_size_buff_tx()
        result = self.get_register_withsize(hw_stat_register_name, buff_size)
        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return {
            "error": False,
            "message": result["message"],
            "data": hardware.read_decode_hw_status(result)
        }

    def get_power(self):
        """
        Le o registrador HW_STAT e retorna os valores de power via HW_TXB.
        """
        hardware = NASCERR_HARDWARE("power")
        hw_stat_register_name = "HW_STAT"
        payload = hardware.write_hw_status_payload()
        write = self.set_register(hw_stat_register_name, payload["data"])

        if write["error"]:
            return {
                "error": True,
                "message": write["message"],
                "data": None
            }
        sleep(0.1)

        hw_stat_register_name = "HW_TXB"
        buff_size = hardware.get_size_buff_tx()
        result = self.get_register_withsize(hw_stat_register_name, buff_size)
        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return {
            "error": False,
            "message": result["message"],
            "data": hardware.read_decode_hw_status(result)
        }

    def set_sleep(self):
        """
        Coloque o dispositivo em modo de Sleep.
        """
        power_manager = NASCERR_Power_Manager()
        payload = power_manager.sleep()

        result = self.set_register("SLEEP", payload)

        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return result

    def set_wakeup(self):
        """
        Acorde o dispositivo do modo de Sleep.
        """
        power_manager = NASCERR_Power_Manager()
        payload = power_manager.wakeup()

        result = self.set_register("WAKEUP", payload)

        if result["error"]:
            return {
                "error": True,
                "message": result["message"],
                "data": None
            }

        return result

    def get_experiment(self) -> dict:
        """
        Lê o(s) resultado(s) de experimento(s) presente(s) no dispositivo.

        Passos:
        1. IDX – 2 bytes de tamanho + 2 bytes de CRC16
        2. TXB – lê exatamente <tamanho> bytes
        3. Valida CRC16 de ambos os registros
        4. Remove CRC de TXB, fatia o payload em blocos de
        NASCERR_EXPERIMENT._SIZE (= 86 bytes) e desserializa
        5. Devolve lista de dicionários

        Não há envio de ACK; em caso de erro gera ValueError.
        """
        sleep(0.01)

        # ---------- IDX ----------
        idx_resp = self.get_register("IDX")
        if idx_resp["error"]:
            return idx_resp

        if len(idx_resp["data"]) != 4:
            raise ValueError("IDX: esperado 4 bytes (size+CRC)")

        if not verify_crc16("".join(f"{b:02X}" for b in idx_resp["data"])):
            raise ValueError("IDX: CRC inválido")

        exp_len = struct.unpack('<H', bytes(idx_resp["data"][:2]))[0]
        print(f"Expecting {exp_len} bytes in TXB")
        # ---------- TXB ----------
        txb_resp = self.get_register_withsize("TXB", exp_len)
        print("data read: ", txb_resp)
        if txb_resp["error"]:
            return txb_resp

        if len(txb_resp["data"]) != exp_len:
            raise ValueError(
                "TXB: numero de bytes diferente do indicado em IDX")

        if not verify_crc16("".join(f"{b:02X}" for b in txb_resp["data"])):
            raise ValueError("TXB: CRC invalido")

        # Remove CRC16 final e desserializa
        payload = bytes(txb_resp["data"][:-2])
        blk_size = NASCERR_EXPERIMENT._SIZE
        if len(payload) % blk_size != 0:
            raise ValueError(
                "TXB: payload noo e multiplo de NASCERR_EXPERIMENT")

        experiments = [
            NASCERR_EXPERIMENT.from_bytes(payload[i:i + blk_size])
            for i in range(0, len(payload), blk_size)
        ]

        return {
            "error":   False,
            "message": txb_resp["message"],
            "data":    [exp.to_dict() for exp in experiments]
        }

    def clear_database(self, region: FlashRegionTypeDef) -> dict:
        """
        Escreve em DB_CLEAR para apagar uma região da flash.

        • `region` deve ser um membro de FlashRegionTypeDef  
        • Payload: 1 byte (região) + 2 bytes CRC16 little-endian
        """
        if not isinstance(region, FlashRegionTypeDef):
            raise TypeError("region precisa ser FlashRegionTypeDef")

        if region == FlashRegionTypeDef.MAX_FLASH_REGION:
            raise ValueError(
                "MAX_FLASH_REGION nao e regiao valida para apagar")

        register = "DB_CLEAR"

        payload = struct.pack("<B", region.value)
        crc16 = calculate_crc16(payload)
        payload += struct.pack("<H", crc16)

        result = self.set_register(register, payload)

        return {
            "error":   result["error"],
            "message": result["message"],
            "data": {
                "region": region.name,
                "value":  region.value,
                "crc16":  f"{crc16:04X}"
            }
        }

    def send_gps_data(self) -> dict:
        """
        Grava coordenadas aleatorias no registrador GPS, seguidos de CRC16.
        tamanho fixo: 34 bytes (GNSS) + 2 bytes de CRC (=36)
        """
        register = "GPS"

        gps = NASCERR_GPS()
        gps._fill_random_fields()

        result = self.set_register(register, gps.to_bytes())

        return {
            "error":   result["error"],
            "message": result["message"],
            "data": gps.to_bytes().hex(':')
        }
