from enum import IntFlag
import enum
import struct
import json
from nascerr_modules.util.crc import calculate_crc16, verify_crc16
from nascerr_modules.nascerr_system import ERROS_LOGS

class BootFlags(IntFlag):
    UPDATE_PENDING     = 1 << 0
    IN_BOOTLOADER      = 1 << 1
    IMAGE_VALID        = 1 << 2
    DOWNLOAD_SUCCESS   = 1 << 3
    RETRANSMIT_REQ     = 1 << 4
    ROLLBACK_AVAILABLE = 1 << 5
    ERROR_DETECTED     = 1 << 6
    IN_DONWLOAD        = 1 << 7 

class BootCurrentApp(enum.IntEnum):
    NONE            = 0x00
    BANK_1          = 0xAA
    BANK_2          = 0xBB

class BootCommand(enum.IntEnum):
    GET_VERSION     = 0x03
    DOWNLOAD_INIT   = 0x01
    DATA_TRANSFER   = 0xAA
    DOWNLOAD_FINISH = 0x02

class NASCERR_BOOT:
    """
    Represents one bootloader command packet:
        [ length (2B big) | cmd (1B) | payload (N bytes) | CRC16 (2B big) ]
    """
    def __init__(self, cmd: BootCommand, payload: bytes = b""):
        self.cmd     = BootCommand(cmd)
        self.payload = payload

    @classmethod
    def make_get_version(cls):
        return cls(BootCommand.GET_VERSION)

    @classmethod
    def make_download_init(cls, fw_size: int, version: int, num_packets: int):
        p = fw_size.to_bytes(4, "big") + version.to_bytes(4, "big") + num_packets.to_bytes(2, "big")
        return cls(BootCommand.DOWNLOAD_INIT, p)

    @classmethod
    def make_data_transfer(cls, packet_id: int, chunk: bytes):
        if not (0 <= packet_id <= 0xFFFF):
            raise ValueError("packet_id must fit in 0-0xFFFF")
        p = packet_id.to_bytes(2, "big") + chunk

        return cls(BootCommand.DATA_TRANSFER, p)

    @classmethod
    def make_download_finish(cls, version: int, sha: bytes):
        """
        Cria o comando DOWNLOAD_FINISH.

        O payload é formatado como: [ version (4B big-endian) | SHA-256 (32B) ]

        :param version: A versão do firmware (inteiro de 32 bits).
        :param sha: O hash SHA-256 do firmware (deve ser um objeto bytes de 32 bytes).
        :raises ValueError: Se a versão for inválida ou o hash não tiver 32 bytes.
        """
        if not (0 <= version <= 0xFFFFFFFF):
            raise ValueError("A versão deve ser um inteiro de 32 bits sem sinal.")

        if not isinstance(sha, bytes) or len(sha) != 32:
            raise ValueError("O hash SHA-256 deve ser um objeto bytes de 32 bytes.")

        # --- Monta o payload: 4 bytes da versão + 32 bytes do hash ---
        p = version.to_bytes(4, "big") + sha

        return cls(BootCommand.DOWNLOAD_FINISH, p)

    def to_bytes(self) -> dict[str, any]:
        """
        Build the raw packet [ len(2B BE) | cmd(1B) | payload | CRC16(2B BE) ]
        and return:
          - message_bytes: the full bytes sequence to send
          - bytes_msg:      total length of message_bytes
          - crc16:          computed CRC16 over [len|cmd|payload]
        """
        length =  (2 + 1 + len(self.payload) + 2).to_bytes(2, "big")
        body   = self.cmd.to_bytes(1, "big") + self.payload
        msg    = length + body

        crc = calculate_crc16(msg)

        packet = msg + crc.to_bytes(2, "big")

        return {
            "message_bytes":  packet,
            "bytes_msg":      len(packet),
            "crc16":          crc
        }

    def to_dict(self) -> dict:
        """
        Retorna um dicionário com:
         - length: tamanho (sem contar CRC)
         - cmd:    nome do comando
         - payload: hex do payload
         - crc16:   hex do CRC
        """
        info = self.to_bytes()
        raw  = info["message_bytes"]  

        return {
            "length":  int.from_bytes(raw[0:2], "big"),
            "cmd":     self.cmd.name,
            "payload": self.payload.hex(),
            "crc16":   raw[-2:].hex()
        }

    def to_json(self, **kwargs) -> str:
        return json.dumps(self.to_dict(), **({"indent": 4} if not kwargs else kwargs))

class NASCERR_STATUS_BOOT:
    """
    Represents the 8-byte BOOT_STAT register:
      [ version (4B BE) | new_version (4B BE) | flags (1B) | last_error (1B) | last_pkg_id (2B BE) | current_app (1B) ]
    """
    __slots__ = ("version", "new_version", "flags", "last_error", "last_packet_id", "current_app")
    
    def __init__(self, version, new_version, flags, last_error, last_packet_id, current_app):
        self.version         = version
        self.new_version     = new_version 
        self.flags           = BootFlags(flags)
        self.last_error      = last_error
        self.last_packet_id  = last_packet_id
        self.current_app     = (
            BootCurrentApp(current_app) if isinstance(current_app, int) and current_app in BootCurrentApp._value2member_map_ 
            else BootCurrentApp.NONE
        )

    @classmethod
    def from_list(cls, data: list[int]) -> "NASCERR_STATUS_BOOT":
        if len(data) < 13:
            raise ValueError("BOOT_STAT payload must be 10 bytes")

        version        = int.from_bytes(bytes(data[0:4]), "big")
        new_version    = int.from_bytes(bytes(data[4:8]), "big")
        flags          = data[8]
        last_error     = data[9]
        last_packet_id = int.from_bytes(bytes(data[10:12]), "big")
        current_app_val = data[12]

        try:
            current_app = BootCurrentApp(current_app_val)
        except ValueError:
            current_app = BootCurrentApp.NONE  # ou `current_app = None` se quiser lidar como valor nulo
        
        return cls(version, new_version, flags, last_error, last_packet_id, current_app)

    def version_str(self, version) -> str:
        """
        Interpret the 32-bit version as three 8-bit fields:
           major = bits 23-16
           minor = bits 15-8
           patch = bits 7-0
        """
        major = (version >> 16) & 0xFF
        minor = (version >> 8)  & 0xFF
        patch = version         & 0xFF
        return f"{major}.{minor}.{patch}"
    
    @property
    def flags_list(self) -> list[str]:
        """Return a list of flag names that are set."""
        return [flag.name for flag in BootFlags if flag & self.flags]

    def to_dict(self) -> dict:
        """
        Return a JSON-serializable dict with:
          - version_raw: the integer as read from hardware
          - version:     human string "major.minor.patch"
          - new_version: the integer new version as read from hardware
          - new_version_raw: the integer as read from hardware
          - flags_raw:   the integer bitmask
          - flags:       list of human-readable names
          - last_error:  the raw error code
          - last_packet_id: the last packet ID received
        """
        return {
            "version_raw": self.version,
            "version":     self.version_str(self.version),
            "new_version_raw": self.new_version,
            "new_version": self.version_str(self.new_version),
            "flags_raw":   int(self.flags),
            "flags":       self.flags_list,
            "last_error": (
                {
                    "name": err.name,
                    "description": err.description
                }
                if (err := ERROS_LOGS._value2member_map_.get(self.last_error))
                else {
                    "name": f"UNKNOWN({self.last_error})",
                    "description": "Unknown error"
                }
            ),
            "current_app": self.current_app.name if isinstance(self.current_app, BootCurrentApp) else "UNKNOWN",
            "last_packet_id": self.last_packet_id
        }

    def to_json(self, **kwargs) -> str:
        return json.dumps(self.to_dict(), **({"indent":4} if not kwargs else kwargs))