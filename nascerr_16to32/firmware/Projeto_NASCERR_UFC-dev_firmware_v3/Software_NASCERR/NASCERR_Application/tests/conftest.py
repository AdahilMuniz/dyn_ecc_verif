import pytest
import os
from nascerr_modules.nascerr_operation import Operation

@pytest.fixture(scope="session")
def device_name():
    """Nome do device I2C (ex.: /dev/i2c-1)."""
    return "/dev/i2c-1"

@pytest.fixture(scope="session")
def i2c_address():
    """Endereço I2C do slave."""
    return 0x55

@pytest.fixture(scope="session")
def operation(device_name, i2c_address):
    """Fixture principal que fornece a instância Operation conectada ao slave."""
    return Operation(device_name, i2c_address)

@pytest.fixture
def dummy_firmware(tmp_path):
    """Cria um firmware de teste de 2 KB."""
    fw_path = tmp_path / "firmware.bin"
    fw_path.write_bytes(os.urandom(2048))
    return str(fw_path)
