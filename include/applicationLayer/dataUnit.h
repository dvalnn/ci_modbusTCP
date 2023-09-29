#ifndef _DATA_UNIT_H_
#define _DATA_UNIT_H_

#include <inttypes.h>

#define MODBUS_DATA_MAX_SIZE 252

/**
 * @brief modbusPDU is a struct that represents a modbus Protocol Data Unit
 *
 * @param fCode function code
 * @param data data of the PDU
 * @param dataLen length of the data - not part of the PDU, but it's useful to have it here
 *
 */
typedef struct t_modbusPDU {
    uint8_t fCode;
    uint8_t* data;
    uint8_t dataLen;
} modbusPDU;

modbusPDU* newModbusPDU(uint8_t fCode, uint8_t* data, uint8_t dataLen);
void freeModbusPDU(modbusPDU* pdu);

int serializeModbusPDU(modbusPDU* pdu, uint8_t* retArray);

#endif  // _DATA_UNIT_H_
