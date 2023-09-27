#include <stdio.h>
#include <stdlib.h>

#include "applicationLayer/modbusApp.h"

#include "log.h"

#include "sds.h"
#include "sdsalloc.h"

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

    sds reply = receiveReply(socketfd, id);
    if (reply == NULL) {
        ERROR("failed to receive reply\n");
        return -1;
    }

    // print reply as hex
    printf("Reply: ");
    for (int i = 0; i < sdslen(reply); i++) {
        printf("%02X ", (uint8_t)reply[i]);
    }
    printf("\n");
    sdsfree(reply);

    closeConnection(socketfd);
    return 0;
}
