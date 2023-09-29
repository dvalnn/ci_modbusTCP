#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <sys/time.h>

#define MODBUS_TIMEOUT_SEC 3
#define MODBUS_TIMEOUT_USEC 0

#define MODBUS_TCP_PORT 502

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
} modbusADU;

int connectToModbusTCP(char* ip, int port, time_t seconds, suseconds_t microseconds);
int disconnectFromModbusTCP(int socketfd);

modbusADU* createModbusADU(uint16_t transactionID, uint8_t* pdu, int pduLen);
void freeModbusADU(modbusADU* adu);

int sendModbusADU(int socketfd, modbusADU* adu);
modbusADU* receiveModbusADU(int socketfd);

#endif  // _MODBUS_TCP_H_