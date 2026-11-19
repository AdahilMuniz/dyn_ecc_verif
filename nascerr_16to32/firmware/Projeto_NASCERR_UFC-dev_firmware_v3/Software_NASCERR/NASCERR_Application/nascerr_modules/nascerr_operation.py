from time import sleep
from nascerr_modules.nascerr_protocol import Protocol, NASCERR_CTRL, NASCERR_CMD
from nascerr_modules.nascerr_system import *
from typing import List, Dict
import os
import time
from nascerr_modules.util.crc import calculate_stm32_compatible_sha256
from math import ceil

SLEEP_TIME = 0.05
CONVERT_TIME = 1
# restrições de envio
MAX_WORD = 1024
MAX_REP = 1  # não utilizado -> verificar necessidade
TIMEOUT_SEC = 20.0  # tempo máximo que vamos esperar por cada pacote


class Operation:
    def __init__(self, device_name, i2c_address):
        self.communication = Protocol(
            device_name=device_name, i2c_address=i2c_address)

    def send_command(self, cmd_type, id, data_lenght, repeat, write_type, data, mode, delay, offset):
        ctrl = NASCERR_CTRL(
            cmd_type=cmd_type,
            id=id,
            data_lenght=data_lenght,
            repeat=repeat,
            write_type=write_type,
            data=data,
            mode=mode,
            # Convertendo minutos para milissegundos
            delay=(delay*CONVERT_TIME),
            offset=offset
        )

        # if(data_lenght>MAX_WORD):
        #     return {"error": True, "status": "fail", "log_error": "Data length exceeds the maximum word limit"}

        # result = self.communication.set_command(
        #     ctrl.cmd_type, ctrl.id, ctrl.data_lenght, ctrl.repeat, ctrl.write_type, ctrl.data, ctrl.mode, ctrl.delay, ctrl.exp_image_ctrl, ctrl.offset)

        result = self.communication.set_command(
            ctrl.cmd_type, ctrl.id, ctrl.data_lenght, ctrl.repeat, ctrl.write_type, ctrl.data, ctrl.mode, ctrl.delay, ctrl.offset)

        sleep(SLEEP_TIME)

        if result.get("error"):
            return {"error": result["error"], "status": "fail", "log_error": result["message"]}

        return {
            "error": result["error"],
            "message": result["message"],
            "command_bytes": result.get("command_bytes", ""),
            "crc16": result.get("crc16", ""),
            "command_dict": result.get("command_dict", {})
        }

    def get_status(self):
        # sleep(SLEEP_TIME)
        try:
            result = self.communication.get_status()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }

        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
                # AINDA NÃO VI UMA APLICACAO DISSO (TALVEZ QUANDO FOR O OBDH MSM)
                "connection_status": "tentando"
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def get_log(self):
        sleep(SLEEP_TIME)
        try:
            result = self.communication.get_log()
        except ValueError as e:
            return {
                "error": "Erro na leitura do log",
                "status": "fail",
                "log_error": str(e),
                "log_status": "falha"
            }
        except Exception as e:
            return {
                "error": "Erro inesperado na leitura do log",
                "status": "fail",
                "log_error": str(e),
                "log_status": "erro"
            }

        if not result:
            return {
                "error": "Nenhum dado de log disponível",
                "status": "empty",
                "log_error": "Lista de logs vazia ou inválida",
                "log_status": "vazio"
            }

        return {
            "error": None,
            "status": "ok",
            "data": result,
            "log_status": "ok"
        }

    def read_result(self):
        try:
            result = self.communication.get_response()
        except Exception as e:
            return {"error": True, "status": "fail", "log_error": f"Exception occurred: {str(e)}"}

        sleep(SLEEP_TIME)
        if result is None:
            return {"error": True, "status": "fail", "log_error": "No response from the device (NoneType)"}

        if result.get("error"):
            return {"error": result["error"], "status": "fail", "log_error": result["message"]}

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"]
        }

    def read_result_byPackages(self, max_retries=3):
        """
        Lê resultados em pacotes, com tentativas de leitura em caso de falha.

        :param max_retries: Número máximo de tentativas por pacote.
        """
        try:
            result = self.communication.get_response()

            if result is None:
                return {"error": True, "status": "fail", "log_error": "No response from the device (NoneType)"}
            if result.get("error"):
                return {"error": result["error"], "status": "fail", "log_error": result["message"]}

            response_data = result["data"]
            number_packages = response_data["number_packages"]
            if number_packages <= 1:  # Se houver apenas 1 pacote, retorna imediatamente
                return {
                    "error": result["error"],
                    "message": result["message"],
                    "data": response_data
                }

            result_array = response_data["result_array"]

            for _ in range(number_packages - 1):
                # print(f"Recebendo pacote {_ + 1}/{number_packages - 1}")
                retries = 0
                while retries < max_retries:
                    status = self.get_status()
                    # print("Primeiro status: ")
                    # print(status)
                    while (1):
                        if status["data"]["request"]["name"] == "REQ_TXRESULT" or status["data"]["operation"]["name"] == "OP_OK":
                            break
                        status = self.get_status()
                        # print(status)
                    next_result = self.communication.get_response()
                    # print("next_result")
                    # print(next_result)
                    if next_result and not next_result.get("error"):
                        # Pacote recebido com sucesso
                        result_array.extend(
                            next_result["data"]["result_array"])
                        break
                    else:
                        retries += 1
                        if retries >= max_retries:
                            return {
                                "error": True,
                                "status": "fail",
                                "log_error": next_result["message"] if next_result else f"Failed to receive package {_} after {max_retries} retries"
                            }
                        # print(f"Retrying to receive package {_} ({retries}/{max_retries})")

            # print("resulte array: ", result_array)
            response_data["result_array"] = result_array
            sleep(SLEEP_TIME)
            return {
                "error": result["error"],
                "message": result["message"],
                "data": response_data
            }
        except Exception as e:
            return {"error": True, "status": "fail", "log_error": f"Exception occurred: {str(e)}"}

    def send_commandNew(
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
        Envia um comando NASCERR_CMD.

        Parâmetros iguais aos campos do NASCERR_CMD.
        """
        # Monta o objeto de comando
        cmd = NASCERR_CMD(
            cmd_type=cmd_type,
            id=id,
            block=block,
            qtd_block=qtd_block,
            ecc=ecc,
            write_type=write_type,
            scheduler_time=scheduler_time * CONVERT_TIME,  # min → ms
            repeat=repeat,
            delay=delay,
            current_tick=current_tick
        )

        # Envia pelo canal de comunicacao
        result = self.communication.set_commandNew(
            cmd.cmd_type,
            cmd.id,
            cmd.block,
            cmd.qtd_block,
            cmd.ecc,
            cmd.write_type,
            cmd.scheduler_time,
            cmd.repeat,
            cmd.delay,
            cmd.current_tick
        )

        sleep(SLEEP_TIME)

        if result.get("error"):
            return {
                "error":  result["error"],
                "status": "fail",
                "log_error": result["message"]
            }

        return {
            "error":   result["error"],
            "message": result["message"],
            "command_bytes": result.get("command_bytes", b""),
            "crc16":   result.get("crc16", 0),
            "command_dict": result.get("command_dict", {})
        }

    def get_experiment_status(self):
        try:
            result = self.communication.get_experiment_status()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }

        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
                "connection_status": "tentando"
            }

        return {
            "error":             result["error"],
            "message":           result["message"],
            "data":              result["data"],
            "connection_status": "conectado"
        }

    def set_exp_sum_idx(self, offset: int):
        try:
            result = self.communication.set_expSumIDX(offset)
        except ValueError as e:
            return {
                "error": True,
                "status": "fail",
                "log_error": str(e)
            }

        if result.get("error"):
            return {
                "error":  result["error"],
                "status": "fail",
                "log_error": result["message"]
            }

        return {
            "error":   result["error"],
            "status":  "ok",
            "message": result["message"]
        }

    def get_exp_sum(self):
        try:
            result = self.communication.get_expSUM()
        except ValueError as e:
            return {
                "error": True,
                "status": "fail",
                "log_error": str(e),
            }

        if result.get("error"):
            return {
                "error":  result["error"],
                "status": "fail",
                "log_error": result["message"],
                "data":    result["data"]
            }

        return {
            "error":   result["error"],
            "status":  "ok",
            "message": result["message"],
            "data":    result["data"]
        }

    def update_firmware(self, firmware_path: str,
                        new_version: int,
                        packet_size: int = 256,
                        progress_cb=None,
                        force: bool = 0) -> Dict:
        """
        Atualiza o firmware do dispositivo.

        :param firmware_path: Caminho para o arquivo de firmware.
        :param new_version: Nova versão do firmware.
        :param packet_size: Tamanho do pacote de dados para envio. 
        :param progress_cb: Função de callback para reportar progresso (opcional).
        :param force: Forçar atualização mesmo se a versão atual for igual ou maior que a nova.
        :return: Dicionário com o relatório da atualização.
        """
        report: Dict = {"steps": []}

        if not os.path.isfile(firmware_path):
            raise FileNotFoundError(f"Firmware not found: {firmware_path}")
        fw = open(firmware_path, "rb").read()

        sha256_hash = calculate_stm32_compatible_sha256(fw)

        total_size = len(fw)  
        num_pkts = ceil(total_size / (packet_size)) 

        stat = self.communication.boot_get_status()
        report["steps"].append({"action": "get_status", "result": stat})

        curr_ver = stat["data"]["version_raw"]
        if curr_ver == new_version and not force:
            report["skipped"] = f"already at version {curr_ver}"
            return report

        init = self.communication.boot_init(
            firmware_size=total_size, version=new_version, num_packets=num_pkts)
        report["steps"].append({"action": "init_download", "result": init})
        if init["error"]:
            return report
        
        start_init = time.time()
        init_stabilized = False
        last_flags = stat["data"]["flags"]
        
        while time.time() - start_init < TIMEOUT_SEC:
            st_init = self.communication.boot_get_status()
            report["steps"].append(
                {"action": "check_init_status", "result": st_init})
            if st_init["error"]:
                return report

            flags = st_init["data"]["flags"]

            if flags != last_flags:
                last_flags = flags
                init_stabilized = True

            if "IN_DONWLOAD" in flags:
                break

            if init_stabilized and "ERROR_DETECTED" in flags:
                return {
                    "error": True,
                    "message": "Device reported error after init",
                    "post_init_status": st_init
                }

            time.sleep(SLEEP_TIME)  

        else:
            return {
                "error": True,
                "message": "Timeout waiting for device to enter bootloader mode",
                "post_init_status": st_init
            }

        step = {"action": "data_transfer", "packets": []}
        report["steps"].append(step)

        total_packets = num_pkts
        retransmitted_packets = 0
        successful_packets = 0

        for idx in range(1, num_pkts + 1):
            chunk = fw[(idx-1)*packet_size: idx*packet_size]
            tx = self.communication.boot_data_transfer(id_pkg=idx, data=chunk)

            entry = {"id": idx, "tx_result": tx}
            start = time.time()
            if tx["error"]:
                report["error_at"] = idx
                step["packets"].append(entry)
                retransmitted_packets += 1
                continue

            while True:
                st = self.communication.boot_get_status()
                if st["error"]:
                    entry["status_error"] = (
                        f"Failed to read BOOT_STAT after packet {idx}: "
                        f"{st['message']}"
                    )
                    report["error_at"] = idx
                    step["packets"].append(entry)
                    return report

                entry["status_after"] = st["data"]
                flags = st["data"]["flags"]
                last_packet_id = st["data"]["last_packet_id"]

                if last_packet_id == idx and "DOWNLOAD_SUCCESS" in flags:
                    successful_packets += 1
                    break

                if "ERROR_DETECTED" in flags:
                    entry["status_error"] = f"Error flag raised after packet {idx}"
                    report["error_at"] = idx
                    step["packets"].append(entry)
                    return report

                if time.time() - start > TIMEOUT_SEC:
                    entry["status_error"] = f"Timeout waiting for pkt {idx}"
                    report["error_at"] = idx
                    step["packets"].append(entry)
                    return report

                # time.sleep(SLEEP_TIME)

            step["packets"].append(entry)

        fin = self.communication.boot_donwload_finish(new_version, 
                                                      sha=sha256_hash)
        
        report["steps"].append({"action": "finish_download", "result": fin})

        total_time = time.time() - start_init

        summary = {
            "version": new_version,
            "total_packets": total_packets,
            "retransmitted_packets": retransmitted_packets,
            "successful_packets": successful_packets,
            "total_firmware_size": total_size,
            "total_time_seconds": total_time,
            "success": "error_at" not in report
        }

        if progress_cb:
            progress_cb(report)

        return summary

    def get_boot_status(self):
        try:
            result = self.communication.boot_get_status()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
                "connection_status": "desconectado" 
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }
        
    def get_hardware_temperature(self):
        """
        Obtem a temperatura do hardware.
        """
        try:
            result = self.communication.get_temperature()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def get_hardware_current(self):
        """
        Obtem a corrente do hardware.
        """
        try:
            result = self.communication.get_current()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def get_hardware_power(self):
        """
        Obtem a potencia do hardware.
        """
        try:
            result = self.communication.get_power()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def get_hardware_total_current(self):
        """
        Obtem a corrente total do hardware.
        """
        try:
            result = self.communication.get_total_current()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def get_hardware_total_power(self):
        """
        Obtem a potencia total do hardware.
        """
        try:
            result = self.communication.get_total_power()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def system_sleep(self):
        """
        Coloca o sistema em modo de sleep.
        """
        try:
            result = self.communication.set_sleep()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "connection_status": "Sleep"
        }

    def system_wakeup(self):
        """
        Desperta o sistema do modo de sleep.
        """
        try:
            result = self.communication.set_wakeup()
        except ValueError as e:
            return {
                "error": "Device not connected",
                "status": "fail",
                "log_error": str(e),
                "connection_status": "desconectado"
            }
        if result.get("error"):
            return {
                "error": result["error"],
                "status": "fail",
                "log_error": result["message"],
                "connection_status": "desconectado"
            }

        return {
            "error": result["error"],
            "message": result["message"],
            "data": result["data"],
            "connection_status": "conectado"
        }

    def read_experiment(self):
        """
        Lê o(s) experimento(s) gravado(s) no dispositivo e devolve-os como
        lista de dicionários NASCERR_EXPERIMENT.

        Retorno:
            {
                "error":   bool       – True se ocorreu erro de barramento/CRC/etc.
                "status":  "ok" | "fail"
                "message": str        – mensagem do driver I²C
                "data":    list[dict] – lista com os experimentos (ou None em erro)
                "log_error": str      – descrição do erro caso status == "fail"
            }
        """
        try:
            result = self.communication.get_experiment()
        except Exception as e:
            return {
                "error": True,
                "status": "fail",
                "log_error": f"Exception occurred: {str(e)}",
                "data": None
            }

        sleep(SLEEP_TIME)

        if result is None:
            return {
                "error": True,
                "status": "fail",
                "log_error": "No response from the device (NoneType)",
                "data": None
            }

        if result.get("error"):
            return {
                "error":  result["error"],
                "status": "fail",
                "log_error": result["message"],
                "data": None
            }

        return {
            "error":   result["error"],
            "status":  "ok",
            "message": result["message"],
            "data":    result["data"]
        }

    def clear_database(self, region: FlashRegionTypeDef):
        """
        Apaga a região indicada na flash chamando Protocol.clear_database().

        Retorno:
            {
                "error":   bool
                "status":  "ok" | "fail"
                "message": str
                "data":    { "region": str, "value": int, "crc16": "ABCD" }
                "log_error": str   (apenas se status == "fail")
            }
        """
        try:
            resp = self.communication.clear_database(region)
        except (ValueError, TypeError) as e:
            return {
                "error": True,
                "status": "fail",
                "log_error": str(e)
            }

        if resp.get("error"):
            return {
                "error":  resp["error"],
                "status": "fail",
                "log_error": resp["message"]
            }

        return {
            "error":   False,
            "status":  "ok",
            "message": resp["message"],
            "data":    resp["data"]
        }

    def gps_data(self) -> dict:
        """
        Envia 34 bytes referentes aos dados de gps + CRC-16 para o registrador GPS.

        Retorno-padrão:
            {
                "error":   bool,
                "status":  "ok" | "fail",
                "message": str,
                "data":    bytes escritos,
                "log_error": str   # somente se status == "fail"
            }
        """
        try:
            resp = self.communication.send_gps_data()
        except Exception as exc:
            return {
                "error": True,
                "status": "fail",
                "log_error": f"Exception occurred: {exc}"
            }

        if resp.get("error"):
            return {
                "error":  resp["error"],
                "status": "fail",
                "log_error": resp["message"]
            }

        return {
            "error":  False,
            "status": "ok",
            "message": resp["message"],
            "data":    resp["data"]
        }
