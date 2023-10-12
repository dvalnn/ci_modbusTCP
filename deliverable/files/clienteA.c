#include <stdio.h>
#include <stdlib.h>

#include "ModbusAP.h"

// comment this line to disable debug messages
// #define DEBUG

#ifdef DEBUG

#define INFO(...) fprintf(stdout, "[INFO] " __VA_ARGS__)
#define ERROR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define MALLOC_ERR \
    ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

#else

#define INFO(...)
#define ERROR(...)
#define MALLOC_ERR

#endif

#define REMOTEHOST "10.277.113.1"
#define LOCALHOST "127.0.0.1"
#define PORT 502

uint16_t *toBigEndian16Bit(uint8_t *data, int len) {
    uint16_t *result = (uint16_t *)malloc(len * sizeof(uint16_t) / 2);
    for (int i = 0; i < len; i++) {
        result[i] = data[i * 2] << 8 | data[i * 2 + 1];
    }
    return result;
}

int checkForExceptions(uint8_t *packet, int pLen, int expectedLen) {
    if (packet == NULL) {
        ERROR("checkForExceptions: packet is NULL\n");
        return -1;
    }

    if (packet[0] & 0x80) {
        ERROR("Exception: %d\n", packet[1]);
        return packet[1];
    }

    if (pLen != expectedLen) {
        ERROR("checkForExceptions: incorrect response lenght\n");
        return -1;
    }

    return 0;
}

int main() {
    int socketfd = connectToServer(LOCALHOST, PORT);
    if (socketfd == -1) {
        ERROR("connect to local host failed\n");
        printf("result = -1\n");
        return -1;
    }

    int rLen1, rLen2, error;
    uint8_t *response1, *response2;
    uint16_t startingAddress, quantity, transactionID;

    startingAddress = 120;  // write to the 121th register
    quantity = 1;
    transactionID = 1;

    //* 1. write 0x41 to the 121th register
    uint16_t data1[] = {0x41};

    response1 = writeMultipleRegisters(socketfd, transactionID, startingAddress,
                                       quantity, data1, &rLen1);
    // expected response length: 5 bytes- 1 for function code, 2 for starting
    // address, 2 for quantity
    error = checkForExceptions(response1, rLen1, 5);
    if (error != 0) {
        ERROR("exit on %s, %d\n", __FILE__, __LINE__);
        printf("result = %d\n", error);
        return -1;
    }

    free(response1);

    INFO("Part 1 done\n");

    //* 2. read values from registers 122-125

    startingAddress = 121;
    quantity = 4;

    transactionID = 2;

    response1 = readHoldingRegisters(socketfd, transactionID, startingAddress,
                                     quantity, &rLen1);
    error = checkForExceptions(response1, rLen1, 10);
    if (error != 0) {
        ERROR("exit on %s, %d\n", __FILE__, __LINE__);
        printf("result = %d\n", error);
        return error;
    }

    INFO("Part 2 done\n");

    //* 3. read the value from register 126
    startingAddress = 125;
    quantity = 1;

    transactionID = 3;

    response2 = readHoldingRegisters(socketfd, transactionID, startingAddress,
                                     quantity, &rLen2);
    error = checkForExceptions(response2, rLen2, 4);
    if (error != 0) {
        ERROR("exit on %s, %d\n", __FILE__, __LINE__);
        printf("result = %d\n", error);
        return error;
    }

    INFO("Part 3 done\n");

    //* 3.5 calculate C
    uint8_t *responseData = &response1[2];
    uint16_t *A = toBigEndian16Bit(responseData, 8);
    uint16_t B = response2[2] << 8 | response2[3];
    uint16_t C = 0;

    if (B == 0) {
        C = 9999;
    } else {
        C = A[0] + A[3];
    }

    free(A);
    free(response1);
    free(response2);

    INFO("Part 3.5 done\n");
    INFO("A = %04X %04X %04X %04X \n", A[0], A[1], A[2], A[3]);
    INFO("B = %04X\n", B);
    INFO("C = %04X\n", C);

    //* 4. write C to register 127 LOCALHOST
    startingAddress = 126;
    quantity = 1;

    transactionID = 4;

    uint16_t data2[] = {C};

    response1 = writeMultipleRegisters(socketfd, transactionID, startingAddress,
                                       quantity, data2, &rLen1);
    error = checkForExceptions(response1, rLen1, 5);
    if (error != 0) {
        ERROR("exit on %s, %d\n", __FILE__, __LINE__);
        printf("result = %d\n", error);
        return error;
    }

    free(response1);

    disconnectFromServer(socketfd);

    INFO("Part 4 done\n");

#pragma region Part 5
    //* 5. Write C to register 128 REMOTEHOST

    socketfd = connectToServer(REMOTEHOST, PORT);
    if (socketfd == -1) {
        ERROR("connect to Remote Host failed\n");
        printf("result = -1\n");
        return -1;
    }

    startingAddress = 126;
    quantity = 1;

    transactionID = 5;

    response1 = writeMultipleRegisters(socketfd, startingAddress, transactionID,
                                       quantity, data2, &rLen1);
    error = checkForExceptions(response1, rLen1, 5);
    if (error != 0) {
        ERROR("exit on %s, %d\n", __FILE__, __LINE__);
        printf("result = %d\n", error);
        return error;
    }

    free(response1);
    disconnectFromServer(socketfd);

#pragma endregion  // Part 5

    printf("result = 0\n");
    return 0;
}