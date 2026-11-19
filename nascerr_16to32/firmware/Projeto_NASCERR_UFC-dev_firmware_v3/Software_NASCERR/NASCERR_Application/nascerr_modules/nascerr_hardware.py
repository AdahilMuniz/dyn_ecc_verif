import struct
from nascerr_modules.util.crc import calculate_crc16, verify_crc16


class NASCERR_HARDWARE:
    """
    Classe para estabelecimento informacao no HW_STAT e
    decodificacao de dados dos registradores HW_TXB.
    """

    _SIZE_TABLE = {
        "temperature": 6,  # 1 float + CRC
        "total_power": 6,
        "power": 22,  # 5 floats + CRC
        "current": 22
    }

    _MESSAGE_TABLE = {
        "temperature": 0xB1,  # 1 float + CRC
        "total_power": 0xB4,

        "current": 0xB2,  # 5 floats + CRC
        "power": 0xB3
    }

    def __init__(self, code: str):
        self.code = code
        self.size_buff_tx = self._SIZE_TABLE.get(code)
        self.message = self._MESSAGE_TABLE.get(code)
        if self.size_buff_tx is None:
            raise ValueError(
                f"Invalid code: {code}. Expected one of {list(self._SIZE_TABLE.keys())}.")
        if self.message is None:
            raise ValueError(
                f"Invalid code: {code}. Expected one of {list(self._MESSAGE_TABLE.keys())}.")

    def get_size_buff_tx(self) -> int:
        """
        Retorna o tamanho do buffer de transmissao com base no codigo.
        """
        return self.size_buff_tx

    def write_hw_status_payload(self) -> dict:
        """
        Cria o payload para escrever no registrador HW_STAT.
        O payload contem o codigo do hardware e o CRC.
        """
        payload = struct.pack("<B", self.message)
        payload += struct.pack(">H", calculate_crc16(payload))
        payload = {
            "register": "HW_STAT",
            "data": list(payload)
        }
        return payload

    def read_decode_hw_status(self, payload: dict) -> dict:
        """
        Le o registrador ``HW_TXB`` e converte bytes em floats.
        O tamanho esperado do payload eh inferido da inicializacao da classe.
        """
        raw = payload.get("data", [])
        raw_hex = ''.join(f'{byte:02X}' for byte in raw)
        if self.size_buff_tx != len(raw):
            raise ValueError("Invalid data length for HW_TXB register.")

        if not verify_crc16(raw_hex):
            raise ValueError("Invalid CRC in HW_TXB register data.")

        # O CRC e os ultimos 2 bytes, então a decodificacao ignora-os.
        data_bytes = bytes(raw[:-2])
        num_floats = len(data_bytes) // 4

        floats = struct.unpack(f"<{num_floats}f", data_bytes)

        payload = list(floats)
        return payload


class NASCERR_Power_Manager:
    # Sleep and Wakeup

    def __init__(self):
        self.status = "AWAKE"

    def sleep(self):
        """
        Cria o payload para colocar o sistema em modo de sleep.
        """
        payload = struct.pack("<B", 0xBB)
        payload += struct.pack(">H", calculate_crc16(payload))
        self.status = "SLEEP"

        return payload

    def wakeup(self):
        """
        Cria o payload para acordar o sistema.
        """
        payload = struct.pack("<B", 0xAA)
        payload += struct.pack(">H", calculate_crc16(payload))
        self.status = "AWAKE"

        return payload
