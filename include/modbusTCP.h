#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int disconnectFromModbusTCP(int socket);
int connectToModbusTCP(char* ip, int port);
int sendModbusRequestTCP(int socket, char* request, int requestLength);
int receiveModbusResponseTCP(int socket, char* response, int responseLength);

#endif