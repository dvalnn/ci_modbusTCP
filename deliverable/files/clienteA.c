#include <stdio.h>
#include <stdlib.h>

#include "ModbusAP.h"

// comment this line to disable debug messages
// #define DEBUG

#ifdef DEBUG

#define ERROR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

#else

#define ERROR(...)
#define MALLOC_ERR

#endif

#define REMOTEHOST "10.277.113.1"
#define LOCALHOST "127.0.0.1"
#define PORT 502

int main() {
    int socketfd = connectToServer(LOCALHOST, PORT);
    if (socketfd == -1) {
        ERROR("connectToServer failed\n");
        printf("result = -1\n");
        return -1;
    }

    int rLen1, rLen2;
    uint8_t *response1, *response2;
    uint16_t startingAddress, quantity, transactionID;

    startingAddress = 120;  // write to the 121th register
    quantity = 1;
    transactionID = 1;

    // 1. write 0x41 to the 121th register
    uint16_t data1[] = {0x41};

    response1 = writeMultipleRegisters(socketfd, startingAddress, transactionID, quantity, data1, &rLen1);
    if (response1 == NULL) {
        ERROR("writeMultipleRegisters failed\n");
        return -1;
    }

    // handle server exeption
    if (response1[0] & 0x80) {
        ERROR("writeMultipleRegisters failed\n");
        printf("result = %d\n", response1[1]);
        return response1[1];
    }

    // check if rLen1 is correct (should be 5 bytes - 1 for function code, 2 for starting address, 2 for quantity)
    if (rLen1 != 5) {
        ERROR("writeMultipleRegisters: incorrect response lenght\n");
        printf("result = -1\n");
        return -1;
    }

    free(response1);

    // 2. read values from registers 122-125

    startingAddress = 121;
    quantity = 4;

    transactionID = 2;

    response1 = readHoldingRegisters(socketfd, startingAddress, transactionID, quantity, &rLen1);
    if (response1 == NULL) {
        ERROR("Read Holding Registers failed\n");
        printf("result = -1\n");
        return -1;
    }

    // handle server exeption
    if (response1[0] & 0x80) {
        ERROR("Read Holding Registers exception\n");
        printf("result = %d\n", response1[1]);
        return response1[1];
    }

    // check if rLen1 is correct (should be 10 - 2 bytes for function code and byte count, 8 bytes for data)
    if (rLen1 != 10) {
        ERROR("Read Holding Registers: incorrect response lenght\n");
        printf("result = -1\n");
        return -1;
    }

    // 3. read the value from register 126
    startingAddress = 125;
    quantity = 1;

    transactionID = 3;

    response2 = readHoldingRegisters(socketfd, startingAddress, transactionID, quantity, &rLen2);
    if (response2 == NULL) {
        ERROR("readHoldingRegisters failed\n");
        printf("result = -1\n");
        return -1;
    }

    // handle server exeption
    if (response2[0] & 0x80) {
        ERROR("readHoldingRegisters exception\n");
        printf("result = %d\n", response2[1]);
        return response2[1];
    }

    // check if rLen2 is correct (should be 4 - 2 bytes for function code and byte count, 2 bytes for data)
    if (rLen2 != 5) {
        ERROR("readHoldingRegisters: incorrect response lenght\n");
        printf("result = -1\n");
        return -1;
    }

    // 3.5 calculate C

    uint16_t A[8] = {0};
    uint16_t B = response2[3] << 8 | response2[4];
    uint16_t C = 0;

    if (B == 0) {
        C = 9999;
    } else {
        // convert response1 to uint16_t array
        for (int i = 2; i < rLen1; i += 2) {
            A[i] = response1[i] << 8 | response1[i + 1];
        }

        C = A[0] + A[3];
    }

    free(response1);
    free(response2);

    // 4. write C to register 127 LOCALHOST
    startingAddress = 126;
    quantity = 1;

    transactionID = 4;

    uint16_t data2[] = {C};

    response1 = writeMultipleRegisters(socketfd, startingAddress, transactionID, quantity, data2, &rLen1);
    if (response1 == NULL) {
        ERROR("writeMultipleRegisters failed\n");
        printf("result = -1\n");
        return -1;
    }

    // handle server exeption
    if (response1[0] & 0x80) {
        ERROR("writeMultipleRegisters exception\n");
        printf("result = %d\n", response1[1]);
        return response1[1];
    }

    // check if rLen1 is correct (should be 5 bytes - 1 for function code, 2 for starting address, 2 for quantity)
    if (rLen1 != 5) {
        ERROR("writeMultipleRegisters: incorrect response lenght\n");
        printf("result = -1\n");
        return -1;
    }

    free(response1);

    disconnectFromServer(socketfd);

    // 5. Write C to register 128 REMOTEHOST

    socketfd = connectToServer(REMOTEHOST, PORT);
    if (socketfd == -1) {
        ERROR("connectToServer failed\n");
        printf("result = -1\n");
        return -1;
    }

    startingAddress = 126;
    quantity = 1;

    transactionID = 5;

    response1 = writeMultipleRegisters(socketfd, startingAddress, transactionID, quantity, data2, &rLen1);
    if (response1 == NULL) {
        ERROR("writeMultipleRegisters failed\n");
        printf("result = -1\n");
        return -1;
    }

    // handle server exeption
    if (response1[0] & 0x80) {
        ERROR("writeMultipleRegisters exception\n");
        printf("result = %d\n", response1[1]);
        return response1[1];
    }

    // check if rLen1 is correct (should be 5 bytes - 1 for function code, 2 for starting address, 2 for quantity)
    if (rLen1 != 5) {
        ERROR("writeMultipleRegisters: incorrect response lenght\n");
        printf("result = -1\n");
        return -1;
    }

    free(response1);

    disconnectFromServer(socketfd);
}