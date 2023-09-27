#include <stdio.h>

#include "include/modbusApp.h"

#include "include/log.h"
#include "include/sds.h"
#include "include/sdsalloc.h"

#define MODBUS_DEFAULT_PORT 502

int main(int argc, char const* argv[]) {
    char ip[] = "127.0.0.1";  // localhost
    int socketfd = openModbusConnection(ip, MODBUS_DEFAULT_PORT);
    uint16_t startingAddress = 0x0000;
    uint16_t quantity = 6;

    if (readHoldingRegisters(socketfd, startingAddress, quantity) < 0) {
        ERROR("failed to read holding registers\n");
        return -1;
    }

    closeModbusConnection(socketfd);
    return 0;
}
