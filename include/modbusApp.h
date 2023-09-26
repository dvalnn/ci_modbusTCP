
#ifndef _MODBUS_APP_H_
#define _MODBUS_APP_H_

#include <inttypes.h>

#include "modbusTCP.h"
#include "sds.h"
#include "sdsalloc.h"

#define PHYSICAL_ADDRESS_MIN 1
#define PHYSICAL_ADDRESS_MAX 65536

#define MODBUS_ADDRESS_MIN 0x0000       // 0
#define MODBUS_ADDRESS_MAX 0xFFFF       // 65535
#define MODBUS_REG_QUANTITY_MIN 0x0001  // 1
#define MODBUS_REG_QUANTITY_MAX 0x007D  // 125

#define MODBUS_PDU_MAX_SIZE 253
#define MODBUS_DATA_MAX_SIZE 252
#define MODBUS_MBAP_HEADER_SIZE 7
typedef enum t_functionCode {
    readHoldingRegsFuncCode = 0x03,    // 3
    writeMultipleRegsFuncCode = 0x10,  // 16
} functionCode;

typedef struct t_modbusMBAP {
    uint16_t transactionIdentifier;
    uint16_t protocolIdentifier;
    uint16_t length;
    uint8_t unitIdentifier;
} modbusMBAP;

typedef struct t_modbusPDU {
    uint8_t fCode;
    uint8_t data[MODBUS_DATA_MAX_SIZE];
    uint8_t byteCount;
} modbusPDU;

typedef struct t_modbusADU {
    modbusPDU pdu;
    modbusMBAP mbapHeader;
} modbusTcpPacket;

#define openModbusConnection(ipString) connectToModbusTCP(ipString)
#define closeModbusConnection(intSocketFD) closeModbusTCP(intSocketFD)

int readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity);

int writeMultipleRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data);

int sendModbusRequest(modbusTcpPacket request, int socketfd);
int receiveModbusResponse(int socketfd, uint16_t transactionID, sds data);
// int receiveModbusResponse(modbusTcpPacket* response, int socketfd);

#endif  // _MODBUS_APP_H_