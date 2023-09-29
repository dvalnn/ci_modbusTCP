
#ifndef _MODBUS_APP_H_
#define _MODBUS_APP_H_

#include <inttypes.h>

#define PHYSICAL_ADDRESS_MIN 1
#define PHYSICAL_ADDRESS_MAX 65536

#define MODBUS_ADDRESS_MIN 0x0000       // 0
#define MODBUS_ADDRESS_MAX 0xFFFF       // 65535
#define MODBUS_REG_QUANTITY_MIN 0x0001  // 1
#define MODBUS_REG_QUANTITY_MAX 0x007D  // 125

#define TIMEOUT_SEC 5
#define TIMEOUT_USEC 0

typedef enum t_functionCode {
    readHoldingRegsFuncCode = 0x03,    // 3
    writeMultipleRegsFuncCode = 0x10,  // 16
} functionCode;

int connectToServer(char* ip, int port);
void disconnectFromServer(int socketfd);

uint8_t* writeMultipleRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data, int* rlen);
uint8_t* readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity, int* rlen);

#endif  // _MODBUS_APP_H_