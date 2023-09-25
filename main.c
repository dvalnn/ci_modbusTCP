#include <stdio.h>

#include "include/modbusTCP.h"
#include "include/sds.h"

#define MODBUS_TCP_MAX_ADU_LENGTH 260

int main(int argc, char const* argv[]) {
    sds ip = sdsnew("127.0.0.1");

    int socketfd = connectToModbusTCP(ip);
    if (socketfd < 0) {
        printf("Error: cannot connect to modbus server\n");
        return -1;
    }
    printf("Connected to modbus server\n");

    // read holding registers
    uint8_t request[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06,
                         0x01, 0x03, 0x00, 0x00, 0x00, 0x02};

    int sent = sendModbusRequestTCP(socketfd, request, sizeof(request));
    if (sent < 0) {
        printf("Error: cannot send request\n");
        return -1;
    }
    printf("Request sent\n");

    // receive response
    uint8_t response[MODBUS_TCP_MAX_ADU_LENGTH];
    int received = recv(socketfd, response, sizeof(response), 0);
    if (received < 0) {
        printf("Error: cannot receive response\n");
        return -1;
    }

    printf("Response received\n");
    printf("Response: ");
    for (int i = 0; i < received; i++) {
        printf("%02x ", response[i]);
    }
    printf("\n");

    printf("Closing connection\n");
    disconnectFromModbusTCP(socketfd);
    printf("Connection closed\n");

    return 0;
}
