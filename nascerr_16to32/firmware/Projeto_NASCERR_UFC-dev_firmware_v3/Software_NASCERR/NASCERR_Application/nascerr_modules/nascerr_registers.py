from dataclasses import dataclass, field
from enum import Enum
from typing import List


class AccessType(Enum):
    WRITE_ONLY = "write only"
    READ_ONLY = "read only"
    FULL_ACCESS = "full access"


# Register addresses, sizes, and descriptions
register_info = {
    "NONE":         {'addr': 0x00, 'size': 0,  'access': AccessType.FULL_ACCESS, 'description': 'No register selected'},
    # 2 bytes for data and 2 bytes for crc
    "STAT":         {'addr': 0x01, 'size': 11,  'access': AccessType.READ_ONLY,   'description': 'Operation status register'},
    # 2 bytes for data and 2 bytes for crc
    "IDX":          {'addr': 0x02, 'size': 4,  'access': AccessType.READ_ONLY,   'description': 'Number of the data block to transmit'},
    "TXB":          {'addr': 0x03, 'size': 2066, 'access': AccessType.READ_ONLY, 'description': 'Transmission buffer register'},
    "CMD":          {'addr': 0x04, 'size': 512, 'access': AccessType.WRITE_ONLY, 'description': 'Command buffer register'},
    "MODE":         {'addr': 0x05, 'size': 1,  'access': AccessType.READ_ONLY,   'description': 'Operation mode register'},
    "SLEEP":        {'addr': 0x06, 'size': 3,  'access': AccessType.WRITE_ONLY,  'description': 'SLEEP mode register'},
    "WAKEUP":       {'addr': 0x07, 'size': 3,  'access': AccessType.WRITE_ONLY,  'description': 'WAKEUP mode register'},
    "GPS":          {'addr': 0x08, 'size': 36, 'access': AccessType.WRITE_ONLY,  'description': 'GPS data register'},
    # "TIME":         {'addr': 0x09, 'size': 6,  'access': AccessType.WRITE_ONLY,  'description': 'TIMESTAMP data register'},
    # "TEMP":         {'addr': 0x0A, 'size': 2,  'access': AccessType.WRITE_ONLY,  'description': 'TEMPERATURE data register'},
    # "ERROR_OP":     {'addr': 0x0C, 'size': 2,  'access': AccessType.READ_ONLY,   'description': 'Operation Error Log'},
    # "ERROR_EPS":    {'addr': 0x0D, 'size': 2,  'access': AccessType.READ_ONLY,   'description': 'Power Supply Error Log'},
    "ACK":          {'addr': 0x0D, 'size': 4,  'access': AccessType.WRITE_ONLY,  'description': 'ACK register'},
    "HW_STAT":      {'addr': 0x0E, 'size': 3,  'access': AccessType.WRITE_ONLY,  'description': 'Hardware status register'},
    "HW_TXB":       {'addr': 0x0F, 'size': 22, 'access': AccessType.READ_ONLY,   'description': 'Hardware transmission buffer register'},
    "BOOT_DATA":    {'addr': 0x10, 'size': 263, 'access': AccessType.WRITE_ONLY, 'description': 'RXB register'},
    "BOOT_STAT":    {'addr': 0x11, 'size': 15,  'access': AccessType.READ_ONLY,  'description': 'Registro de status BOOT'},
    "LOG":          {'addr': 0x12, 'size': 128, 'access': AccessType.READ_ONLY,  'description': 'LOG data register'},
    "EXP_STAT":     {'addr': 0x13, 'size': 18, 'access': AccessType.READ_ONLY,   'description': 'Experiment status buffer register'},
    "EXP_SUM_IDX":  {'addr': 0x14, 'size': 3,  'access': AccessType.WRITE_ONLY,  'description': 'Page/offset selector for unread-summary page'},
    "EXP_SUM_DATA": {'addr': 0x15, 'size': 216, 'access': AccessType.READ_ONLY,   'description': 'Unread experiments summary page'},
    "DB_CLEAR":     {'addr': 0x16, 'size': 3,  'access': AccessType.WRITE_ONLY,  'description': 'Database-erase command register'},
}


def validate_access(access):
    if not isinstance(access, AccessType):
        raise ValueError("access must be an instance of AccessType")


def validate_addr(addr):
    if not isinstance(addr, int) or addr < 0:
        raise ValueError("addr must be a positive integer")


def validate_size(size):
    if not isinstance(size, int) or size < 0:
        raise ValueError("size must be a positive integer")


@dataclass
class Register:
    access: AccessType
    addr: int
    size: int
    description: str
    data: List[int] = field(init=False)

    def __post_init__(self):
        validate_access(self.access)
        validate_addr(self.addr)
        validate_size(self.size)
        self.data = [0] * self.size  # Inicializa o vetor de dados com zeros

    def get_addr_hex(self):
        return hex(self.addr)

    def __repr__(self):
        return (f"Register(access={self.access}, addr=0x{self.addr:X}, size={self.size}, "
                f"description='{self.description}', data={self.data})")

    def __str__(self):
        return (f"Register with access type {self.access.value}, address 0x{self.addr:X}, "
                f"size {self.size}, description: {self.description}, data {self.data}")


# Allocation of memory for register values
registers = {name: Register(
    info['access'], info['addr'], info['size'], info['description']) for name, info in register_info.items()}
