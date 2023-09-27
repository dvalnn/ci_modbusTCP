# !/usr/bin/env python3

import argparse
import logging
from datetime import datetime
from pyModbusTCP.server import ModbusServer, DataBank


class MyDataBank(DataBank):
    """A custom ModbusServerDataBank for override get_holding_registers method."""

    def __init__(self):
        # turn off allocation of memory for standard modbus object types
        # only "holding registers" space will be replaced by dynamic build values.
        super().__init__(virtual_mode=True)

    def get_holding_registers(self, address, number=1, srv_info=None):
        """Get virtual holding registers."""
        # populate virtual registers dict with current datetime values
        now = datetime.now()
        v_regs_d = {
            0: now.day,
            1: now.month,
            2: now.year,
            3: now.hour,
            4: now.minute,
            5: now.second,
            6: 0,
            7: 0,
            8: 0,
            9: 0,
        }
        # build a list of virtual regs to return to server data handler
        # return None if any of virtual registers is missing
        try:
            return [v_regs_d[a] for a in range(address, address + number)]
        except KeyError:
            return


if __name__ == "__main__":
    # parse args
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-H", "--host", type=str, default="localhost", help="Host (default: localhost)"
    )
    parser.add_argument(
        "-p", "--port", type=int, default=502, help="TCP port (default: 502)"
    )
    args = parser.parse_args()

    # init modbus server and start it
    server = ModbusServer(
        host=args.host, port=args.port, data_bank=MyDataBank(), no_block=True
    )

    try:
        print("ModbusTCP server starting...")
        server.start()
        print("Server online")
        while True:
            continue
    except:
        print("\nServer shutting down...")
        server.stop()
        print("ModbusTCP server offline")
