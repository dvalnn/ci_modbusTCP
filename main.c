#include <stdio.h>
#include <stdlib.h>

#include "applicationLayer/modbusApp.h"

#include "log.h"

#include "sds.h"
#include "sdsalloc.h"

void printStringAsHex(char* s, int len) {
    printf("Reply: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", (uint8_t)s[i]);
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

    int socketfd = openConnection(ip, port);
    if (socketfd < 0) {
        ERROR("cannot open socket\n");
        return -1;
    }

    uint16_t startingAddress = 0x0000;
    uint16_t quantity = 6;
    printf("sending Read Holding Registers request\n");
    printf("starting address: %d, quantity: %d\n", startingAddress, quantity);

    int id = sendReadHoldingRegs(socketfd, startingAddress, quantity);
    if (id < 0) {
        ERROR("failed to send Read Holding Registers request\n");
        return -1;
    }

    sds reply = receiveReply(socketfd, id, readHoldingRegsFuncCode);
    if (reply == NULL) {
        ERROR("failed to receive reply\n");
        return -1;
    }

    // print reply as hex
    printStringAsHex(reply, sdslen(reply));

    quantity = 4;
    startingAddress = 6;
    uint16_t data[4] = {0x0001, 0x0002, 0x0003, 0x0004};
    printf("sending Write Multiple Registers request\n");
    printf("starting address: %d, quantity: %d\n", startingAddress, quantity);
    printf("data: ");
    for (int i = 0; i < quantity; i++) {
        printf("%04X ", data[i]);
    }
    printf("\n");

    id = sendWriteMultipleRegs(socketfd, startingAddress, quantity, data);
    if (id < 0) {
        ERROR("failed to send Read Holding Registers request\n");
        return -1;
    }

    reply = receiveReply(socketfd, id, writeMultipleRegsFuncCode);
    if (reply == NULL) {
        ERROR("failed to receive reply\n");
        // return -1;
    } else
        printStringAsHex(reply, sdslen(reply));

    quantity = 9;
    startingAddress = 0;
    printf("sending Read Holding Registers request\n");
    printf("starting address: %d, quantity: %d\n", startingAddress, quantity);
    id = sendReadHoldingRegs(socketfd, startingAddress, quantity);
    if (id < 0) {
        ERROR("failed to send Read Holding Registers request\n");
        return -1;
    }

    reply = receiveReply(socketfd, id, readHoldingRegsFuncCode);
    if (reply == NULL) {
        ERROR("failed to receive reply\n");
        return -1;
    }
    printStringAsHex(reply, sdslen(reply));

    sdsfree(reply);
    closeConnection(socketfd);
    return 0;
}
