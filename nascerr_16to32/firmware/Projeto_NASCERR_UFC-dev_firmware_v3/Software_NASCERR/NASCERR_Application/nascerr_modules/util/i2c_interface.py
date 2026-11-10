from smbus2 import SMBus, i2c_msg
from time import sleep
import weakref, atexit

# Mantém referências fracas a todas as instâncias para fechar no exit
_instances = weakref.WeakSet()

def _close_all_i2c():
    for inst in list(_instances):
        try:
            inst.close()
        except Exception:
            pass

atexit.register(_close_all_i2c)


class I2CInterface:
    def __init__(self, addr=None, addr_default=None, bus=1):
        self.addr = addr if addr is not None else addr_default
        self.bus_id = bus
        self.bus = SMBus(self.bus_id)
        _instances.add(self)

    def write_data(self, cmd, data):
        try:
            if isinstance(data, (bytes, bytearray)):
                data = list(data)

            write_bytes = data if cmd is None else [cmd] + data
            self.bus.i2c_rdwr(i2c_msg.write(self.addr, write_bytes))
            sleep(0.00005)

            return {"error": False, "message": "Data written successfully", "data": None}
        except Exception as e:
            return {"error": True, "message": f"Error: {e}", "data": None}

    def read_data(self, cmd, data_len):
        try:
            msgs = []
            if cmd is not None:
                msgs.append(i2c_msg.write(self.addr, [cmd]))
            msgs.append(i2c_msg.read(self.addr, data_len))
            self.bus.i2c_rdwr(*msgs)

            data = list(msgs[-1])
            return {"error": False, "message": "Data read successfully", "data": data}
        except Exception as e:
            return {"error": True, "message": f"Error: {e}", "data": None}

    def close(self):
        if self.bus:
            self.bus.close()
            self.bus = None

    def restart_i2c_bus(self):
        try:
            self.close()
            sleep(0.05)
            self.bus = SMBus(self.bus_id)
            return {"error": False, "message": "I2C bus restarted successfully"}
        except Exception as e:
            return {"error": True, "message": f"Error restarting the I2C bus: {str(e)}"}
