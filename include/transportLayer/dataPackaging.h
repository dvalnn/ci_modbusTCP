#ifndef _DATA_PACKAGING_H_
#define _DATA_PACKAGING_H_

#include <inttypes.h>

/**
 * @brief modbus ADU (Application Data Unit) structure
 *
 * @param transactionID transaction identifier
 *      (used to associate the future response with this request)
 * @param protocolIdentifier protocol identifier
 *     (0 for Modbus/TCP)
 * @param length number of remaining bytes in this request
 *    (lenght of the PDU + the unit identifier field)
 * @param unitIdentifier unit identifier
 *    (0 for Modbus/TCP)
 * @param pdu protocol data unit
 *   (function code + data)
 *
 * @see https://en.wikipedia.org/wiki/Modbus
 */
typedef struct _modbusADU {
    uint16_t transactionID;
    uint16_t protocolIdentifier;
    uint16_t length;
    uint8_t unitIdentifier;
    uint8_t* pdu;
} ModbusADU;

ModbusADU* newModbusADU(uint16_t transactionID, uint8_t* pdu, int pduLen);
void freeModbusADU(ModbusADU* adu);

int sendModbusADU(int socketfd, ModbusADU* adu);
ModbusADU* receiveModbusADU(int socketfd);

#define UNIT_ID 1
#define PROTOCOL_ID 0

#endif  // _MODBUS_DATA_PACKAGING_H_