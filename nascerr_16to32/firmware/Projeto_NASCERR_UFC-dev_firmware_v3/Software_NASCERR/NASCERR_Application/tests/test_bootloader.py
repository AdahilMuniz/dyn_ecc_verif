import random
from nascerr_modules.nascerr_bootloader import NASCERR_BOOT, BootCommand
from time import sleep
import time

SLEEP_BETWEEN_OPERATIONS = 0.1  
SLEEP_TIMEOUT = 6.0 # TODO: ajustar de acordo com o tempo de timeout do slave
SLEEP_BOOT = 40.0  # tempo para o bootloader reiniciar o sistema

class FaultInjector:
    """
    Classe auxiliar para injetar falhas do lado do master,
    enviando pacotes inválidos ou fora da ordem para o slave.
    """

    def __init__(self, operation):
        self.op = operation

    def send_corrupted_crc(self, packet_id: int, data: bytes):
        """
        Envia um pacote de dados para o bootloader, mas com CRC corrompido.
        Pré-condição: processo de boot_init já deve ter sido realizado.
        """
        pkt = self.op.communication.boot_data_transfer(packet_id, data)
        pkt_bytes = bytearray.fromhex(pkt["command_bytes"])
        sleep(SLEEP_BETWEEN_OPERATIONS)
        pkt_bytes[-1] ^= 0xFF  

        return self.op.communication.set_register("BOOT_DATA", bytes(pkt_bytes))

    def send_out_of_order(self, fw: bytes, packet_order: list[int], pkt_size=256):
        for idx in packet_order:
            chunk = fw[(idx - 1) * pkt_size: idx * pkt_size]
            sleep(SLEEP_BETWEEN_OPERATIONS)
            self.op.communication.boot_data_transfer(idx, chunk)

    def send_incomplete(self, fw: bytes, stop_at: int, pkt_size=256):
        for idx in range(1, stop_at + 1):
            chunk = fw[(idx - 1) * pkt_size: idx * pkt_size]
            sleep(SLEEP_BETWEEN_OPERATIONS)
            self.op.communication.boot_data_transfer(idx, chunk)

    def send_wrong_sha(self, version: int):
        fake_sha = bytes([0xAA] * 32)
        return self.op.communication.boot_donwload_finish(version, fake_sha)

    def send_command_storm(self, n: int = 20, pkt_size=256):
        fw = bytes([0xAB] * (pkt_size * 3))  # firmware dummy de 3 pacotes

        for i in range(n):
            choice = random.choice(["INIT", "DATA", "FINISH"])
            sleep(SLEEP_BETWEEN_OPERATIONS)
            if choice == "INIT":
                fw_size = random.randint(0, len(fw) * 2)
                num_pkts = random.randint(1, 10)
                self.op.communication.boot_init(
                    firmware_size=fw_size,
                    version=random.randint(1, 0xFFFFFF),
                    num_packets=num_pkts
                )
            elif choice == "DATA":
                pkt_id = random.randint(1, 5)
                chunk = fw[0:pkt_size]
                self.op.communication.boot_data_transfer(pkt_id, chunk)
            elif choice == "FINISH":
                fake_sha = bytes([random.randint(0, 255) for _ in range(32)])
                version = random.randint(1, 0xFFFFFF)
                self.op.communication.boot_donwload_finish(version, fake_sha)

def test_command_storm(operation):
    injector = FaultInjector(operation)

    injector.send_command_storm(n=100)

    time.sleep(SLEEP_BETWEEN_OPERATIONS)
    st = operation.communication.boot_get_status()
    assert not st["error"]
    assert "ERROR_DETECTED" in st["data"]["flags"]

def test_crc_corrupted_packet(operation, dummy_firmware): # esse teste tem o msm resultado do queda de comunicação
    injector = FaultInjector(operation)
    fw = open(dummy_firmware, "rb").read()

    total_size = len(fw)
    num_pkts = (total_size // 256) + 1

    init = operation.communication.boot_init(
        firmware_size=total_size,
        version=0xAABBCC,
        num_packets=num_pkts
    )
    assert not init["error"]
    sleep(SLEEP_BETWEEN_OPERATIONS)
    first_chunk = fw[0:256]
    ok_resp = operation.communication.boot_data_transfer(1, first_chunk)
    assert not ok_resp["error"]
    sleep(SLEEP_BETWEEN_OPERATIONS)
    corrupted_chunk = fw[256:512]
    bad_resp = injector.send_corrupted_crc(2, corrupted_chunk)
    assert not bad_resp["error"]  
    sleep(SLEEP_TIMEOUT)
    st = operation.communication.boot_get_status()
    assert "ERROR_DETECTED" in st["data"]["flags"]

def test_incomplete_firmware(operation, dummy_firmware):
    injector = FaultInjector(operation)
    fw = open(dummy_firmware, "rb").read()

    total_size = len(fw)
    num_pkts = (total_size // 256) + 1

    operation.communication.boot_init(firmware_size=total_size,
                                      version=0x112233,
                                      num_packets=num_pkts)
    sleep(SLEEP_BETWEEN_OPERATIONS)
    injector.send_incomplete(fw, stop_at=num_pkts // 2)
    sleep(SLEEP_BETWEEN_OPERATIONS)
    st = operation.get_boot_status()
    assert st["data"]["version_raw"] != 0x112233
    assert "IMAGE_VALID" not in st["data"]["flags"]

def test_wrong_sha_finish(operation, dummy_firmware):
    injector = FaultInjector(operation)
    fw = open(dummy_firmware, "rb").read()

    total_size = len(fw)
    num_pkts = (total_size // 256) + 1

    operation.communication.boot_init(firmware_size=total_size,
                                      version=0x556677,
                                      num_packets=num_pkts)
    sleep(SLEEP_BETWEEN_OPERATIONS)
    for i in range(1, num_pkts + 1):
        chunk = fw[(i - 1) * 256: i * 256]
        sleep(SLEEP_BETWEEN_OPERATIONS)
        operation.communication.boot_data_transfer(i, chunk)
    sleep(SLEEP_BETWEEN_OPERATIONS)
    injector.send_wrong_sha(0x556677)
    sleep(SLEEP_TIMEOUT)
    st = operation.get_boot_status()
    assert "IMAGE_VALID" not in st["data"]["flags"]
    assert st["data"]["version_raw"] != 0x556677
    assert st["data"]["new_version_raw"] != 0x556677

def test_out_of_order_packets(operation, dummy_firmware):
    injector = FaultInjector(operation)
    fw = open(dummy_firmware, "rb").read()

    total_size = len(fw)
    num_pkts = 3

    operation.communication.boot_init(firmware_size=total_size,
                                      version=0x334455,
                                      num_packets=num_pkts)
    sleep(SLEEP_BETWEEN_OPERATIONS)
    injector.send_out_of_order(fw, packet_order=[2, 1, 3])
    sleep(SLEEP_TIMEOUT)
    st = operation.get_boot_status()
    assert "ERROR_DETECTED" in st["data"]["flags"]