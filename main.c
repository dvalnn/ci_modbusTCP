#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

    sds reply;
    int id = 1;
    uint16_t readQuantity = 10;
    uint16_t readAddress = 0;
    
    uint16_t writeAddress = 7;
    uint16_t writeValue = 0;
    uint16_t writeQuantity = 1;

    for (;;) {
        printf("\nRead Holding Registers request\n");
        printf("starting address: %d, quantity: %d\n", readAddress, readQuantity);
        id = sendReadHoldingRegs(socketfd, readAddress, readQuantity);
        if (id < 0) {
            ERROR("cannot send R.H.R request\n");
            break;
        }
        reply = receiveReply(socketfd, id, readHoldingRegsFuncCode);
        if (reply == NULL) {
            ERROR("cannot receive reply\n");
            break;
        } else {
            printStringAsHex(reply, sdslen(reply));
            sdsfree(reply);
        }

        printf("\nWrite Single Register request\n");
        printf("address: %d, value: %d\n", writeAddress, writeValue);
        id = sendWriteMultipleRegs(socketfd, writeAddress, writeQuantity, &writeValue);
        if (id < 0) {
            ERROR("cannot snd W.M.R. request\n");
            break;
        }
        reply = receiveReply(socketfd, id, writeMultipleRegsFuncCode);
        if (reply == NULL) {
            ERROR("cannot receive wmr reply\n");
            break;
        } else {
            printStringAsHex(reply, sdslen(reply));
            sdsfree(reply);
        }

        writeValue = (writeValue + 1) % 0xFFFF;
        sleep(1);
    }

    closeConnection(socketfd);
    return 0;
}
