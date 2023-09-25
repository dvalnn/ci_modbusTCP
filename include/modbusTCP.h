#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MODBUS_TCP_PORT 502

int connectToModbusTCP(char* ip);
int disconnectFromModbusTCP(int socket);
int sendModbusRequestTCP(int socket, uint8_t* request, int requestLength);
int receiveModbusResponseTCP(int socket, uint8_t* response, int responseLength);

#endif