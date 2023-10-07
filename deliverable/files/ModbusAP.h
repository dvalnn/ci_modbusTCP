
#ifndef _MODBUS_AP_H_
#define _MODBUS_AP_H_

#include <inttypes.h>

#define MODBUS_ADDRESS_MIN 0x0000       // 0
#define MODBUS_ADDRESS_MAX 0xFFFF       // 65535
#define MODBUS_QUANTITY_MIN 0x0001      // 1
#define MODBUS_RHR_QUANTITY_MAX 0x007D  // 125
#define MODBUS_WMR_QUANTITY_MAX 0x007B  // 123

#define TIMEOUT_SEC 5
#define TIMEOUT_USEC 0

typedef enum t_functionCode {
    readHoldingRegsFuncCode = 0x03,    // 3
    writeMultipleRegsFuncCode = 0x10,  // 16
} functionCode;

int connectToServer(char* ip, int port);
void disconnectFromServer(int socketfd);

uint8_t* writeMultipleRegisters(int socketfd, uint16_t startingAddress, uint16_t id, uint16_t quantity, uint16_t* data, int* rlen);
uint8_t* readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t id, uint16_t quantity, int* rlen);

#endif  // _MODBUS_AP_H_