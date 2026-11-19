import enum
import json
import struct
from typing import List
from nascerr_modules.nascerr_command import CmdTypeDef
from nascerr_modules.nascerr_system import *

EXP_UNREAD_PAGE_SIZE = 30


class DBStatus(enum.IntEnum):
    """Estados possiveis do banco de experimentos (mirror do enum C)."""
    EMPTY = 0x00
    NOT_READ = 0x01
    READ = 0x02
    CANCELED = 0x03
    NEW_COMMAND = 0x04
    EXECUTED = 0x05


class DescriptionError:
    def __init__(self, addr, result):
        """
        Define a description error structure.
        :param addr: Address (32 bits).
        :param result: Result (0 - No errors, 1 - Bit error).
        """
        self.addr = addr  # 32-bit address
        self.result = result  # 0 - No errors, 1 - Bit error

    def to_bytes(self):
        """
        Serialize the DescriptionError structure to a bytes object.
        """
        return struct.pack('<I I', self.addr, self.result)

    def to_dict(self):
        """
        Convert the DescriptionError structure to a dictionary.
        """
        # Converte o resultado para binário com espaços entre os bits
        result_binary = ' '.join(f'{self.result:032b}'[i] for i in range(32))

        return {
            "addr": f'0x{self.addr:08X}',
            "result": result_binary
        }

    def to_json(self):
        """
        Convert the DescriptionError structure to a JSON string.
        """
        return json.dumps(self.to_dict(), indent=4)


class NASCERR_RESPONSE:
    def __init__(self, id, data_length, number_errors, corrected_data, percent_error, command, number_packages, package_number, result_array, error_by_package):
        """
        Define the NASCERR_RESPONSE structure.
        :param id: Identifier for the response message.
        :param data_length: Length of the data in blocks.
        :param number_errors: Number of detected errors.
        :param corrected_data: Amount of data that was corrected.
        :param result_array: Array of DescriptionError.
        """
        self.id = id  # 1 byte
        self.data_length = data_length  # 4 bytes
        self.number_errors = number_errors  # 4 bytes
        self.corrected_data = corrected_data  # 4 bytes
        self.percent_error = percent_error  # 4 bytes
        self.command = command  # 1 byte
        self.number_packages = number_packages  # 2 byte
        self.package_number = package_number  # 2 byte
        self.error_by_package = error_by_package  # 2 byte
        self.result_array = result_array  # Pointer to array of DescriptionError

    def to_bytes(self):
        """
        Serialize the NASCERR_RESPONSE structure to a bytes object.
        """
        # Serialize the basic fields
        message = struct.pack(
            '<B I I I f B H H H',
            self.id, self.data_length,
            self.number_errors,
            self.corrected_data,
            self.percent_error,
            self.command,
            self.number_packages,
            self.package_number,
            self.error_by_package)

        # Serialize the result array
        for result in self.result_array:
            message += result.to_bytes()

        return message

    def to_dict(self):
        """
        Convert the NASCERR_RESPONSE structure to a dictionary.
        """
        return {
            "id": self.id,
            "data_length": self.data_length,
            "number_errors": self.number_errors,
            "corrected_data": self.corrected_data,
            "percent_error": self.percent_error,
            "number_packages": self.number_packages,
            "package_number": self.package_number,
            "error_by_package": self.error_by_package,
            "command": CmdTypeDef(self.command).name,
            "result_array": [result.to_dict() for result in self.result_array]
        }

    def to_json(self):
        """
        Convert the NASCERR_RESPONSE structure to a JSON string.
        """
        return json.dumps(self.to_dict(), indent=4)


class DBItem:
    """
    Mirror do struct DBItem (7 bytes).

        uint32_t id        -> 4 bytes
        uint16_t num_block -> 2 bytes
        uint8_t  status    -> 1 byte
    """
    _FMT = "<IHB"
    _SIZE = struct.calcsize(_FMT)

    def __init__(self, *, id: int, num_block: int, status: DBStatus):
        self.id = id
        self.num_block = num_block
        self.status = DBStatus(status)

    @classmethod
    def from_bytes(cls, data: bytes) -> "DBItem":
        if len(data) != cls._SIZE:
            raise ValueError(
                f"DBItem espera {cls._SIZE} bytes, recebeu {len(data)}")
        id_, num_block, st = struct.unpack(cls._FMT, data)
        return cls(id=id_, num_block=num_block, status=DBStatus(st))

    def to_bytes(self) -> bytes:
        return struct.pack(self._FMT, self.id, self.num_block, self.status)

    def to_dict(self) -> dict:
        return {
            "id":         self.id,
            "num_block":  self.num_block,
            "status":     self.status.name,
        }

    def __repr__(self):
        return (f"DBItem(id={self.id}, num_block={self.num_block}, "
                f"status={self.status.name})")


class NASCERR_EXP_SUMMARY:
    """
    Mirror do struct ExpUnreadSummary_t.

        uint16_t qtd_unread_total  (2B)
        uint8_t  offset            (1B)
        uint8_t  qtd_included      (1B)
        DBItem   entries[...]      (até 30 × 7B)
    """
    _HDR_FMT = "<HBB"
    _HDR_SIZE = struct.calcsize(_HDR_FMT)

    def __init__(self, *, qtd_unread_total: int, offset: int,
                 entries: List[DBItem]) -> None:

        if len(entries) > EXP_UNREAD_PAGE_SIZE:
            raise ValueError("entries excede EXP_UNREAD_PAGE_SIZE")
        self.qtd_unread_total = qtd_unread_total
        self.offset = offset
        self.qtd_included = len(entries)
        self.entries = entries

    @classmethod
    def from_bytes(cls, data: bytes) -> "NASCERR_EXP_SUMMARY":
        if len(data) < cls._HDR_SIZE:
            raise ValueError("payload muito curto para NASCERR_EXP_SUMMARY")

        total, offset, qtd_included = struct.unpack_from(cls._HDR_FMT, data)
        expect = cls._HDR_SIZE + qtd_included * DBItem._SIZE

        if len(data) < expect:
            raise ValueError("payload incompleto para todos os DBItem")

        items = []
        cursor = cls._HDR_SIZE
        for _ in range(qtd_included):
            chunk = data[cursor: cursor + DBItem._SIZE]
            items.append(DBItem.from_bytes(chunk))
            cursor += DBItem._SIZE

        return cls(qtd_unread_total=total, offset=offset, entries=items)

    def to_bytes(self) -> bytes:
        header = struct.pack(self._HDR_FMT,
                             self.qtd_unread_total,
                             self.offset,
                             self.qtd_included)
        body = b"".join(item.to_bytes() for item in self.entries)

        pad = (EXP_UNREAD_PAGE_SIZE - len(self.entries)) * DBItem._SIZE
        body += b"\xFF" * pad
        return header + body

    def to_dict(self) -> dict:
        return {
            "qtd_unread_total": self.qtd_unread_total,
            "offset":           self.offset,
            "qtd_included":     self.qtd_included,
            "entries":          [e.to_dict() for e in self.entries],
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=4)

    def __repr__(self):
        return (f"NASCERR_EXP_SUMMARY(total={self.qtd_unread_total}, "
                f"offset={self.offset}, included={self.qtd_included}, "
                f"entries={self.entries})")


class MDCA_Data:
    """
    Mirror do struct MDCA_Data_t (34 bytes).

        uint16_t gnss_week          -> 2 bytes
        uint32_t gnss_tow           -> 4 bytes
        float    latitude           -> 4 bytes
        float    longitude          -> 4 bytes
        float    altitude_ellipsoid -> 4 bytes
        float    altitude_msl       -> 4 bytes
        float    ecef_x             -> 4 bytes
        float    ecef_y             -> 4 bytes
        float    ecef_z             -> 4 bytes
    """
    _FMT = "<H I f f f f f f f"
    _SIZE = struct.calcsize(_FMT)

    _FMT  = "<H I f f f f f f f"
    _SIZE = struct.calcsize(_FMT)

    def __init__(self,
                 gnss_week: int, gnss_tow: int,
                 latitude: float, longitude: float,
                 altitude_ellipsoid: float, altitude_msl: float,
                 ecef_x: float, ecef_y: float, ecef_z: float):
        self.gnss_week = gnss_week
        self.gnss_tow = gnss_tow
        self.latitude = latitude
        self.longitude = longitude
        self.altitude_ellipsoid = altitude_ellipsoid
        self.altitude_msl = altitude_msl
        self.ecef_x = ecef_x
        self.ecef_y = ecef_y
        self.ecef_z = ecef_z

    @classmethod
    def from_bytes(cls, data: bytes) -> "MDCA_Data":
        if len(data) != cls._SIZE:
            raise ValueError(
                f"MDCA_Data espera {cls._SIZE} bytes, recebeu {len(data)}")
        vals = struct.unpack(cls._FMT, data)
        return cls(*vals)

    def to_bytes(self) -> bytes:
        return struct.pack(self._FMT, self.gnss_week, self.gnss_tow,
                           self.latitude, self.longitude,
                           self.altitude_ellipsoid, self.altitude_msl,
                           self.ecef_x, self.ecef_y, self.ecef_z)

    def to_dict(self) -> dict:
        return vars(self)

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=4)

    def __repr__(self):
        return f"MDCA_Data({self.to_dict()})"


class ECC_Telemetry:
    """
    Mirror do struct ECC_Telemetry_t (30 bytes).

        uint32_t single_bit_errors    -> 4 bytes
        uint32_t double_bit_errors    -> 4 bytes
        uint32_t triple_bit_errors    -> 4 bytes
        uint32_t beg_region_errors    -> 4 bytes
        uint32_t mid_region_errors    -> 4 bytes
        uint32_t end_region_errors    -> 4 bytes
        uint32_t uncorrectable_errors -> 4 bytes
        uint8_t  ecc                  -> 1 byte
        uint8_t  ecc_status           -> 1 byte
    """
    _FMT  = "<7I B B"
    _SIZE = struct.calcsize(_FMT)

    def __init__(self,
                 single_bit_errors: int, double_bit_errors: int,
                 triple_bit_errors: int, beg_region_errors: int,
                 mid_region_errors: int, end_region_errors: int,
                 uncorrectable_errors: int, ecc: int, ecc_status: int):
        self.single_bit_errors    = single_bit_errors
        self.double_bit_errors = double_bit_errors
        self.triple_bit_errors = triple_bit_errors
        self.beg_region_errors = beg_region_errors
        self.mid_region_errors = mid_region_errors
        self.end_region_errors = end_region_errors
        self.uncorrectable_errors = uncorrectable_errors
        self.ecc = NASCERR_MODE(ecc)
        self.ecc_status = ecc_status  # 0 = OK, 1 = WARN, 2 = FAIL

    @classmethod
    def from_bytes(cls, data: bytes) -> "ECC_Telemetry":
        if len(data) != cls._SIZE:
            raise ValueError(
                f"ECC_Telemetry espera {cls._SIZE} bytes, recebeu {len(data)}")
        vals = struct.unpack(cls._FMT, data)
        return cls(*vals)

    def to_bytes(self) -> bytes:
        return struct.pack(self._FMT,
                           self.single_bit_errors, self.double_bit_errors,
                           self.triple_bit_errors, self.beg_region_errors,
                           self.mid_region_errors, self.end_region_errors,
                           self.uncorrectable_errors, self.ecc, self.ecc_status)

    def to_dict(self) -> dict:
        d = vars(self).copy()
        d["ecc"] = self.ecc.name
        return d

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=4)

    def __repr__(self):
        return f"ECC_Telemetry({self.to_dict()})"


# ---------------------------------------------------------------------------
#  Struct mirror: NASCERR_EXPERIMENT (88 bytes)
# ---------------------------------------------------------------------------
class NASCERR_EXPERIMENT:
    """
    Mirror do struct NASCERR_EXPERIMENT (88 bytes).

        uint32_t id              -> 4 bytes
        uint16_t block           -> 2 bytes
        uint16_t data_lenght     -> 2 bytes
        uint32_t time_delta      -> 4 bytes   (Δt entre escrita e leitura)
        uint32_t number_errors   -> 4 bytes
        uint32_t corrected_data  -> 4 bytes
        float    percent_error   -> 4 bytes
        ECC_Telemetry ecc_telemetry -> 30 bytes
        MDCA_Data   mdca_data       -> 34 bytes
    """
    _HDR_FMT  = "<I H H I I I f"          # 24 bytes de cabeçalho primitivo
    _HDR_SIZE = struct.calcsize(_HDR_FMT)
    _SIZE     = _HDR_SIZE + ECC_Telemetry._SIZE + MDCA_Data._SIZE  # 88 bytes

    def __init__(self, id: int, block: int, data_lenght: int,
                 time_delta: int, number_errors: int, corrected_data: int,
                 percent_error: float, ecc_telemetry: ECC_Telemetry,
                 mdca_data: MDCA_Data) -> None:
        self.id             = id
        self.block          = block
        self.data_lenght    = data_lenght
        self.time_delta     = time_delta
        self.number_errors  = number_errors
        self.corrected_data = corrected_data
        self.percent_error  = percent_error
        self.ecc_telemetry  = ecc_telemetry
        self.mdca_data      = mdca_data

    # -------- (de)serialização --------
    @classmethod
    def from_bytes(cls, data: bytes) -> "NASCERR_EXPERIMENT":
        if len(data) != cls._SIZE:
            raise ValueError(
                f"NASCERR_EXPERIMENT espera {cls._SIZE} bytes, recebeu {len(data)}")
        hdr_vals = struct.unpack_from(cls._HDR_FMT, data)
        cursor   = cls._HDR_SIZE

        ecc  = ECC_Telemetry.from_bytes(
            data[cursor : cursor + ECC_Telemetry._SIZE])
        cursor += ECC_Telemetry._SIZE

        mdca = MDCA_Data.from_bytes(data[cursor : cursor + MDCA_Data._SIZE])

        return cls(
            id             = hdr_vals[0],
            block          = hdr_vals[1],
            data_lenght    = hdr_vals[2],
            time_delta     = hdr_vals[3],
            number_errors  = hdr_vals[4],
            corrected_data = hdr_vals[5],
            percent_error  = hdr_vals[6],
            ecc_telemetry  = ecc,
            mdca_data      = mdca
        )

    def to_bytes(self) -> bytes:
        hdr = struct.pack(
            self._HDR_FMT,
            self.id, self.block, self.data_lenght,
            self.time_delta, self.number_errors,
            self.corrected_data, self.percent_error
        )
        return hdr + self.ecc_telemetry.to_bytes() + self.mdca_data.to_bytes()

    # -------- helpers --------
    def to_dict(self) -> dict:
        return {
            "id":              self.id,
            "block":           self.block,
            "data_lenght":     self.data_lenght,
            "time_delta":      self.time_delta,
            "number_errors":   self.number_errors,
            "corrected_data":  self.corrected_data,
            "percent_error":   self.percent_error,
            "ecc_telemetry":   self.ecc_telemetry.to_dict(),
            "mdca_data":       self.mdca_data.to_dict(),
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=4)

    def __repr__(self):
        def __repr__(self):
            return (
                f"NASCERR_EXPERIMENT(id={self.id}, "
                f"block={self.block}, "
                f"time_delta={self.time_delta} ms …)"
            )
        
