#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#define MODBUS_TIMEOUT_SEC 3
#define MODBUS_TIMEOUT_USEC 0

int disconnectFromModbusTCP(int socket);
int connectToModbusTCP(char* ip, int port);
int sendModbusRequestTCP(int socket, uint8_t* request, int requestLength);
int receiveModbusResponseTCP(int socket, uint8_t* response, int responseLength);

#endif