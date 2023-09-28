# !/usr/bin/env python3

import argparse
import logging
from datetime import datetime
from pyModbusTCP.server import ModbusServer, DataBank

v_regs_d = [0 for _ in range(10)]


class MyDataBank(DataBank):
    """A custom ModbusServerDataBank for override get_holding_registers method."""

    def __init__(self):
        # turn off allocation of memory for standard modbus object types
        # only "holding registers" space will be replaced by dynamic build values.
        super().__init__(virtual_mode=True, h_regs_size=10, h_regs_default_value=0)

    def get_holding_registers(self, address, number=1, srv_info=None):
        """Get virtual holding registers."""
        now = datetime.now()
        # update datetime values every seconds
        v_regs_d[0] = now.day
        v_regs_d[1] = now.month
        v_regs_d[2] = now.year
        v_regs_d[3] = now.hour
        v_regs_d[4] = now.minute
        v_regs_d[5] = now.second

        # build a list of virtual regs to return to server data handler
        # return None if any of virtual registers is missing
        try:
            return [v_regs_d[a] for a in range(address, address + number)]
        except KeyError:
            return

    # set data to holding registers
    def set_holding_registers(self, address, word_list, srv_info=None):
        """Set virtual holding registers."""
        try:
            for a in range(address, address + len(word_list)):
                v_regs_d[a] = word_list[a - address]
            return True
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

    # set logging level
    logging.basicConfig(format="%(asctime)s %(message)s", level=logging.INFO)

    # init modbus server and start it
    server = ModbusServer(host=args.host, port=args.port, data_bank=MyDataBank())
    logging.info("Start server...")
    server.start()
