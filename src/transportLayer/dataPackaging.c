#include "transportLayer/dataPackaging.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "transportLayer/tcpControl.h"

#define MODBUS_MBAP_HEADER_SIZE 7
#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

/**
 * @brief create a modbus ADU instance
 *
 * @param transactionID transaction identifier
 * @param protocolIdentifier protocol identifier (0 for Modbus/TCP)
 * @param unitIdentifier unit identifier (0 for Modbus/TCP)
 * @param pdu protocol data unit
 * @param pduLen protocol data unit length
 * @return modbusADU* pointer to the created modbus ADU
 */
ModbusADU* _newModbusADU(uint16_t transactionID,
                         uint16_t protocolIdentifier,
                         uint8_t unitIdentifier,
                         uint8_t* pdu,
                         int pduLen) {
    ModbusADU* adu = (ModbusADU*)malloc(sizeof(ModbusADU));
    if (adu == NULL) {
        MALLOC_ERR;
        return NULL;
    }

    adu->transactionID = transactionID;
    adu->protocolIdentifier = protocolIdentifier;
    adu->length = pduLen + 1;  // +1 for unit identifier
    adu->unitIdentifier = unitIdentifier;

    if (pduLen <= 0 || pdu == NULL) {
        adu->pdu = NULL;
        return adu;
    }

    adu->pdu = (uint8_t*)malloc(pduLen);
    if (adu->pdu == NULL) {
        MALLOC_ERR;
        return NULL;
    }
    memcpy(adu->pdu, pdu, pduLen);

    return adu;
}

/**
 * @brief Create a Modbus ADU (Application Data Unit) instance
 *
 * @param transactionID transaction identifier
 * @param pdu protocol data unit (function code + data, must be at least 1 byte)
 * @param pduLen protocol data unit length (must be at least 1)
 * @return modbusADU* pointer to the created modbus ADU
 *         NUll if error
 */
ModbusADU* newModbusADU(uint16_t transactionID, uint8_t* pdu, int pduLen) {
    if (transactionID < 0 || pdu == NULL || pduLen <= 0) {
        ERROR("newModbusADU: invalid parameters\n\ttransactionID: %d, pdu: %p, pduLen: %d\n",
              transactionID, pdu, pduLen);
        return NULL;
    }

    return _newModbusADU(transactionID, PROTOCOL_ID, UNIT_ID, pdu, pduLen);
}

/**
 * @brief free a modbus ADU instance allocated with newModbusADU
 *
 * @param adu pointer to the modbus ADU to free
 */
void freeModbusADU(ModbusADU* adu) {
    if (adu == NULL)
        return;

    free(adu->pdu);
    free(adu);
}

/**
 * @brief send a modbus ADU through TCP
 *
 * @param socketfd socket file descriptor
 * @param adu modbus ADU to send
 * @return sent bytes if success, -1 if error
 */
int sendModbusADU(int socketfd, ModbusADU* adu) {
    if (socketfd < 0 || adu == NULL) {
        ERROR("sendModbusADU: invalid parameters\n");
        return -1;
    }

    // create the MBAP header
    uint8_t mbapHeader[MODBUS_MBAP_HEADER_SIZE];
    mbapHeader[0] = (uint8_t)((adu->transactionID >> 8) & 0xFF);
    mbapHeader[1] = (uint8_t)(adu->transactionID & 0xFF);
    mbapHeader[2] = (uint8_t)((adu->protocolIdentifier >> 8) & 0xFF);
    mbapHeader[3] = (uint8_t)(adu->protocolIdentifier & 0xFF);
    mbapHeader[4] = (uint8_t)((adu->length >> 8) & 0xFF);
    mbapHeader[5] = (uint8_t)(adu->length & 0xFF);
    mbapHeader[6] = (uint8_t)(adu->unitIdentifier);

    // pdu length = adu->length - unit identifier
    int pduLen = adu->length - 1;

    // concatenate the MBAP header and the PDU
    // the MBAP header is not included in the length field
    uint8_t* packet = (uint8_t*)malloc((MODBUS_MBAP_HEADER_SIZE + pduLen) * sizeof(*packet));
    if (packet == NULL) {
        MALLOC_ERR;
        return -1;
    }

    memcpy(packet, mbapHeader, MODBUS_MBAP_HEADER_SIZE);
    memcpy(packet + MODBUS_MBAP_HEADER_SIZE, adu->pdu, pduLen);

    // send the packet
    int sent = tcpSend(socketfd, packet, MODBUS_MBAP_HEADER_SIZE + pduLen);
    if (sent < 0) {
        ERROR("Cannot send modbus ADU\n");
        return -1;
    }

    free(packet);

    return sent - MODBUS_MBAP_HEADER_SIZE;
}

/**
 * @brief receive a modbus ADU through TCP
 *
 * @param socketfd socket file descriptor
 * @return modbusADU* pointer to the received modbus ADU, NULL if error
 */
ModbusADU* receiveModbusADU(int socketfd) {
    if (socketfd < 0) {
        ERROR("receiveModbusADU: invalid parameters\n");
        return NULL;
    }

    ModbusADU* adu = _newModbusADU(0, 0, 0, NULL, 0);

    // receive the MBAP header
    uint8_t mbapHeader[MODBUS_MBAP_HEADER_SIZE];
    int received = tcpReceive(socketfd, mbapHeader, MODBUS_MBAP_HEADER_SIZE);
    if (received < 0) {
        ERROR("Cannot receive modbus ADU\n");
        return NULL;
    }

    // parse the MBAP header
    adu->transactionID = (uint16_t)((mbapHeader[0] << 8) | mbapHeader[1]);
    adu->protocolIdentifier = (uint16_t)((mbapHeader[2] << 8) | mbapHeader[3]);
    adu->length = (uint16_t)((mbapHeader[4] << 8) | mbapHeader[5]);
    adu->unitIdentifier = mbapHeader[6];

    // pdu length = adu->length - unit identifier
    int pduLen = adu->length - 1;

    // receive the PDU
    adu->pdu = (uint8_t*)malloc(pduLen);
    if (adu->pdu == NULL) {
        MALLOC_ERR;
        return NULL;
    }

    // receive the PDU (function code + data)
    received = tcpReceive(socketfd, adu->pdu, pduLen);
    if (received < 0) {
        ERROR("Cannot receive modbus data\n");
        return NULL;
    }

    return adu;
}

#undef MALOC_ERR