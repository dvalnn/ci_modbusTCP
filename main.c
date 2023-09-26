#include <stdio.h>

#include "include/modbusApp.h"

#include "include/sds.h"
#include "include/sdsalloc.h"

#define MODBUS_DEFAULT_PORT 502

int main(int argc, char const* argv[]) {
    sds ip = sdsnew("127.0.0.1");

    int socketfd = openModbusConnection(ip, MODBUS_DEFAULT_PORT);
    uint16_t startingAddress = 0x0000;
    uint16_t quantity = 4;

    if (readHoldingRegisters(socketfd, startingAddress, quantity) < 0) {
        fprintf(stderr, "Error: failed to read holding registers\n");
        return -1;
    }

    closeModbusConnection(socketfd);
    return 0;
}
