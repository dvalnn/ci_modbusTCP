#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "applicationLayer/modbusApp.h"
#include "log.h"

void printArrayAsHex(uint16_t* s, int len) {
    printf("Reply: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", (uint16_t)s[i]);
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);

    int socketfd = connectToServer(ip, port);
    if (socketfd < 0) {
        ERROR("cannot connect to server\n");
        return -1;
    }

    uint16_t readQuantity = 10;
    uint16_t readAddress = 0;

    uint16_t writeAddress = 7;
    uint16_t writeValue = 0;
    uint16_t writeQuantity = 1;

    uint16_t* buffer = NULL;
    int bufferLength = 0;
    for (;;) {
        printf("\nRead Holding Registers request\n");
        printf("starting address: %d, quantity: %d\n", readAddress, readQuantity);
        bufferLength = readHoldingRegisters(socketfd, readAddress, readQuantity, buffer);
        if (bufferLength < 0) {
            ERROR("Read Holding Registers failed\n");
            break;
        }
        printArrayAsHex(buffer, bufferLength);

        free(buffer);
        buffer = NULL;

        printf("\nWrite Single Register request\n");
        printf("address: %d, value: %d\n", writeAddress, writeValue);
        bufferLength = writeMultipleRegisters(socketfd, writeAddress, writeQuantity, &writeValue, buffer);
        if (bufferLength < 0) {
            ERROR("Write Single Register failed\n");
            break;
        }

        writeValue = (writeValue + 1) % 0xFFFF;
        sleep(1);
    }

    disconnectFromServer(socketfd);
    return 0;
}
