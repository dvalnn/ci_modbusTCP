#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_


int disconnectFromModbusTCP(int socket);
int connectToModbusTCP(char* ip, int port);
int sendModbusRequestTCP(int socket, char* request, int requestLength);
int receiveModbusResponseTCP(int socket, char* response, int responseLength);

#endif