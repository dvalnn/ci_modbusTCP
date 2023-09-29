#include "transportLayer/modbusConectionManager.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "transportLayer/modbusTCP.h"

#define TRANSMISSION_QEUE_SIZE 10

modbusADU transmissionQeue[TRANSMISSION_QEUE_SIZE];
int transmissionQeueHead = 0;
int transmissionQeueTail = 0;
sem_t transmissionQeueSem;
int currTransactionID = 0;

int connectModbus(char *ip, int port, time_t seconds, suseconds_t microseconds) {
    int socketfd = connectToModbusTCP(ip, port, seconds, microseconds);
    if (socketfd < 0) {
        printf("Error connecting to Modbus server\n");
        return -1;
    }

    if (sem_init(&transmissionQeueSem, 0, TRANSMISSION_QEUE_SIZE) < 0) {
        printf("Error initializing transmission qeue semaphore\n");
        return -1;
    }

    return socketfd;
}