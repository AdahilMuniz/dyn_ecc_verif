from enum import Enum
import struct

class ImageTypeDef(Enum):
    IMG_FF = 0xFF
    IMG_0 = 0  # Selects one of the available images
    IMG_1 = 1
    IMG_2 = 2
    IMG_3 = 3

class FilterTypeDef(Enum):
    FILTER_FF = 0xFF
    FILTER_0 = 0  # Selects one of the available filters
    FILTER_1 = 1
    FILTER_2 = 2

class CompTypeDef(Enum):
    LOSS_FF = 0xFF
    LOSSLESS = 0  # Selects the type of compression used
    LOSSY = 1

class ExpImageCtrl:
    def __init__(self, image: ImageTypeDef, filter: FilterTypeDef, compression: CompTypeDef):
        self.image = image
        self.filter = filter
        self.compression = compression

    def to_bytes(self):
        return struct.pack(
            '<B B B',  # Image, Filter, and Compression packed
            self.image.value,
            self.filter.value,
            self.compression.value
        )

    def to_dict(self):
        return {
            "image": self.image.name,
            "filter": self.filter.name,
            "compression": self.compression.name
        }