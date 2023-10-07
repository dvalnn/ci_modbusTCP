#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <inttypes.h>
#include <sys/time.h>

#define MODBUS_TIMEOUT_SEC 1
#define MODBUS_TIMEOUT_USEC 0

#define MODBUS_TCP_PORT 502

int modbusConnect(char* ip, int port, time_t seconds, suseconds_t microseconds);
int modbusDisconnect(int socketfd);

int modbusSend(int socketfd, uint16_t id, uint8_t* pdu, int pLen);
uint8_t* modbusReceive(int socketfd, uint16_t id, int* pduLen);

#endif  // _MODBUS_TCP_H_