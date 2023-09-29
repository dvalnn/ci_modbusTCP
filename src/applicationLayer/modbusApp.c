#include "applicationLayer/modbusApp.h"

#include <stdio.h>
#include <stdlib.h>

#include "applicationLayer/dataUnit.h"
#include "log.h"
#include "sds.h"
#include "sdsalloc.h"
#include "transportLayer/modbusTCP.h"

#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

/**
 * @brief Connect to the server
 *
 * @param ip
 * @param port
 *
 * @return socket file descriptor, -1 if error
 */
int connectToServer(char* ip, int port) {
    return modbusConnect(ip, port, TIMEOUT_SEC, TIMEOUT_USEC);
}

/**
 * @brief Disconnect from the server
 *
 * @param socketfd
 */
void disconnectFromServer(int socketfd) {
    modbusDisconnect(socketfd);
}

/**
 * @brief create a new modbusPDU formated with a Read Holding Registers request
 *
 * @param startingAddress
 * @param quantity
 * @param transactionID
 * @return modbusPDU*
 */
modbusPDU* newReadHoldingRegs(uint16_t startingAddress, uint16_t quantity) {
    int dataLength = 4;  // 2 bytes for starting address + 2 bytes for quantity
    uint8_t fCode = readHoldingRegsFuncCode;
    uint8_t dataBuf[dataLength];

    dataBuf[0] = (uint8_t)startingAddress >> 8;
    dataBuf[1] = (uint8_t)startingAddress & 0xFF;
    dataBuf[2] = (uint8_t)quantity >> 8;
    dataBuf[3] = (uint8_t)quantity & 0xFF;

    modbusPDU* pdu = newModbusPDU(fCode, dataBuf, dataLength);
    if (pdu == NULL)
        return NULL;

    return pdu;
}

/**
 * @brief send a Read Holding Registers request to the server
 *
 * @param socketfd socket file descriptor
 * @param startingAddress starting address of the registers to read
 * @param quantity number of registers to read
 * @param response pointer to the response buffer
 * @return int response length if success, -1 if error
 */
int readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* response) {
    if (socketfd < 0) {
        ERROR("invalid socket\n");
        return -1;
    }

    if (quantity < MODBUS_REG_QUANTITY_MIN || quantity > MODBUS_REG_QUANTITY_MAX) {
        ERROR("quantity must be between %d and %d\n", MODBUS_REG_QUANTITY_MIN, MODBUS_REG_QUANTITY_MAX);
        return -1;
    }

    if (startingAddress < MODBUS_ADDRESS_MIN || startingAddress > MODBUS_ADDRESS_MAX) {
        ERROR("starting address must be between %d and %d\n", MODBUS_ADDRESS_MIN, MODBUS_ADDRESS_MAX);
        return -1;
    }

    if (startingAddress + quantity > MODBUS_ADDRESS_MAX) {
        ERROR("starting address + quantity must be less than %d\n", MODBUS_ADDRESS_MAX);
        return -1;
    }

    modbusPDU* pdu = newReadHoldingRegs(startingAddress, quantity);
    if (pdu == NULL) {
        return -1;
    }

    int len, sent;
    uint8_t* packet = NULL;
    uint16_t id = readHoldingRegsFuncCode;
    len = serializeModbusPDU(pdu, packet);
    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);
    freeModbusPDU(pdu);

    if (len != sent) {
        ERROR("failed to send Read Holding Registers request\n");
        return -1;
    }

    // receive response
    uint8_t* rBuffer = NULL;
    len = modbusReceive(socketfd, id, rBuffer);

    if (len < 0) {
        ERROR("failed to receive Read Holding Registers response\n");
        return -1;
    }

    // convert byte array to uint16_t array
    int responseLength = len / 2;
    response = malloc(responseLength * sizeof(uint16_t));
    if (response == NULL) {
        MALLOC_ERR;
        return -1;
    }

    for (int i = 0; i < responseLength; i++) {
        response[i] = (uint16_t)rBuffer[2 * i] << 8 | (uint16_t)rBuffer[2 * i + 1];
    }

    return responseLength;
}

/**
 * @brief create a new modbusPDU formated with a Write Multiple Registers request
 *
 * @param startingAddress starting address of the registers to write
 * @param quantity number of registers to write
 * @param data pointer to the data to write
 * @return modbusPDU* pointer to the modbusPDU instance
 */
modbusPDU* newWriteMultipleRegs(uint16_t startingAddress, uint16_t quantity, uint16_t* data) {
    int dataLength = quantity * 2;  // 2 bytes for starting address + 2 bytes for quantity
    uint8_t fCode = writeMultipleRegsFuncCode;
    uint8_t dataBuf[dataLength];

    dataBuf[0] = (uint8_t)startingAddress >> 8;
    dataBuf[1] = (uint8_t)startingAddress & 0xFF;
    dataBuf[2] = (uint8_t)quantity >> 8;
    dataBuf[3] = (uint8_t)quantity & 0xFF;
    dataBuf[4] = (uint8_t)quantity * 2;

    // set pdu request data in Big Endian format
    for (int i = 0; i < quantity; i++) {
        dataBuf[2 * i + 5] = (uint8_t)data[i] >> 8;    // high byte
        dataBuf[2 * i + 6] = (uint8_t)data[i] & 0xFF;  // low byte
    }

    modbusPDU* pdu = newModbusPDU(fCode, dataBuf, dataLength);
    if (pdu == NULL)
        return NULL;

    return pdu;
}

int writeMultipleRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data, uint16_t* response) {
    if (socketfd < 0) {
        ERROR("invalid socket\n");
        return -1;
    }

    if (quantity < MODBUS_REG_QUANTITY_MIN || quantity > MODBUS_REG_QUANTITY_MAX) {
        ERROR("quantity must be between %d and %d\n", MODBUS_REG_QUANTITY_MIN, MODBUS_REG_QUANTITY_MAX);
        return -1;
    }

    if (startingAddress < MODBUS_ADDRESS_MIN || startingAddress > MODBUS_ADDRESS_MAX) {
        ERROR("starting address must be between %d and %d\n", MODBUS_ADDRESS_MIN, MODBUS_ADDRESS_MAX);
        return -1;
    }

    if (startingAddress + quantity > MODBUS_ADDRESS_MAX) {
        ERROR("starting address + quantity must be less than %d\n", MODBUS_ADDRESS_MAX);
        return -1;
    }

    modbusPDU* pdu = newWriteMultipleRegs(startingAddress, quantity, data);
    if (pdu == NULL) {
        return -1;
    }

    int len, sent;
    uint8_t* packet = NULL;
    uint16_t id = writeMultipleRegsFuncCode;
    len = serializeModbusPDU(pdu, packet);
    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);
    freeModbusPDU(pdu);

    if (len != sent) {
        ERROR("failed to send Write Multiple Registers request\n");
        return -1;
    }

    // receive response
    uint8_t* rBuffer = NULL;
    len = modbusReceive(socketfd, id, rBuffer);

    if (len < 0) {
        ERROR("failed to receive Write Multiple Registers response\n");
        return -1;
    }

    // convert byte array to uint16_t array
    int responseLength = len / 2;
    response = malloc(responseLength * sizeof(uint16_t));
    if (response == NULL) {
        MALLOC_ERR;
        return -1;
    }

    for (int i = 0; i < responseLength; i++) {
        response[i] = (uint16_t)rBuffer[2 * i] << 8 | (uint16_t)rBuffer[2 * i + 1];
    }

    return responseLength;
}

#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)