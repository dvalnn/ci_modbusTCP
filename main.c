#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "applicationLayer/modbusApp.h"
#include "log.h"

void printArrayAsHex(void* s, int len) {
    printf("8bHex: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", ((uint8_t*)s)[i]);
    }
    printf("\n");
}

void printByteArrayAsLongHex(void* s, int len) {
    printf("16bHex: ");
    for (int i = 0; i < len; i += 2) {
        printf("%02X ", ((uint8_t*)s)[i] << 8 | ((uint8_t*)s)[i + 1]);
    }
    printf("\n");
}

void printByteArrayAsLongDec(void* s, int len) {
    printf("Dec: ");
    for (int i = 0; i < len; i += 2) {
        printf("%d ", ((uint8_t*)s)[i] << 8 | ((uint8_t*)s)[i + 1]);
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

    uint8_t* buffer = NULL;

    uint16_t transactionID = 0;

    int retval = 0;

    int bufferLength = 0;
    for (;;) {
        printf("\nRead Holding Registers request\n");
        printf("starting address: %d, quantity: %d\n", readAddress, readQuantity);

        transactionID++;
        buffer = readHoldingRegisters(socketfd, readAddress, transactionID, readQuantity, &bufferLength);
        if (buffer == NULL) {
            ERROR("Read Holding Registers failed\n");
            break;
        }
        if (buffer[0] & 0x80) {
            ERROR("Exeption %d code: %d\n", buffer[0], buffer[1]);
            retval = buffer[1];
            break;
        }
        printArrayAsHex(buffer, bufferLength);
        printByteArrayAsLongHex(buffer + 2, bufferLength - 2);  // skip header
        printByteArrayAsLongDec(buffer + 2, bufferLength - 2);

        free(buffer);

        printf("\nWrite Single Register request\n");
        printf("address: %d, value: %d\n", writeAddress, writeValue);

        transactionID++;
        buffer = writeMultipleRegisters(socketfd, writeAddress, transactionID, writeQuantity, &writeValue, &bufferLength);
        if (buffer == NULL) {
            ERROR("Write Single Register failed\n");
            break;
        }
        if (buffer[0] & 0x80) {
            ERROR("Exeption %d code: %d\n", buffer[0], buffer[1]);
            retval = buffer[1];
            break;
        }
        printArrayAsHex(buffer, bufferLength);
        free(buffer);

        writeValue = (writeValue + 1) % 0xFFFF;
        sleep(1);
    }

    disconnectFromServer(socketfd);
    return retval;
}
