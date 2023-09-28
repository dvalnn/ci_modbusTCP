from pyModbusTCP.client import ModbusClient
from time import sleep

if __name__ == "__main__":
    try:
        # define modbus server host, port
        SERVER_HOST = "localhost"
        client = ModbusClient(host=SERVER_HOST, port=502, auto_open=True)
        i = 0
        address = 7
        while True:
            client.write_multiple_registers(address, [i])
            regs = client.read_holding_registers(0, 10)
            print(regs)
            i += 1
            sleep(1)
    except KeyboardInterrupt:
        pass
