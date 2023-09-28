
#ifndef _MODBUS_APP_H_
#define _MODBUS_APP_H_

#include <inttypes.h>

#include "sds.h"
#include "sdsalloc.h"

#include "linkLayer/modbusTCP.h"

#define PHYSICAL_ADDRESS_MIN 1
#define PHYSICAL_ADDRESS_MAX 65536

#define MODBUS_ADDRESS_MIN 0x0000       // 0
#define MODBUS_ADDRESS_MAX 0xFFFF       // 65535
#define MODBUS_REG_QUANTITY_MIN 0x0001  // 1
#define MODBUS_REG_QUANTITY_MAX 0x007D  // 125

#define MODBUS_ADU_MAX_SIZE 260
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
    uint16_t length;  // length of the PDU + 1 (unit identifier)
    uint8_t unitIdentifier;
} modbusMBAP;

typedef struct t_modbusPDU {
    uint8_t fCode;
    uint8_t* data;
    uint8_t dataLen;  // not part of the PDU, but it's useful to have it here
} modbusPDU;

typedef struct t_modbusADU {
    modbusPDU pdu;
    modbusMBAP mbapHeader;
} modbusPacket;

#define openConnection(ipString, port) connectToModbusTCP(ipString, port)
#define closeConnection(intSocketFD) disconnectFromModbusTCP(intSocketFD)

int sendReadHoldingRegs(int socketfd, uint16_t startingAddress, uint16_t quantity);
int sendWriteMultipleRegs(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data);
sds receiveReply(int socketfd, uint16_t transactionID, uint8_t functionCode);

sds flatenPacketToString(modbusPacket packet);

#endif  // _MODBUS_APP_H_