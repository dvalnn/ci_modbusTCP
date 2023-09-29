#include "applicationLayer/modbusApp.h"

#include <stdio.h>
#include <stdlib.h>

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
 * @brief
 *
 * @param startingAddress
 * @param quantity
 * @param len
 * @return uint8_t*
 */
uint8_t* newReadHoldingRegs(uint16_t startingAddress, uint16_t quantity, int* len) {
    *len = 5;  // 1 byte for function code + 2 bytes for starting address + 2 bytes for quantity
    uint8_t* pdu = (uint8_t*)malloc(*len);
    if (pdu == NULL) {
        MALLOC_ERR;
        return NULL;
    }

    pdu[0] = (uint8_t)readHoldingRegsFuncCode;
    pdu[1] = (uint8_t)startingAddress >> 8;
    pdu[2] = (uint8_t)startingAddress & 0xFF;
    pdu[3] = (uint8_t)quantity >> 8;
    pdu[4] = (uint8_t)quantity & 0xFF;

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

    int len, sent, id;
    uint8_t* packet = newReadHoldingRegs(startingAddress, quantity, &len);
    if (packet == NULL) {
        return -1;
    }
    id = readHoldingRegsFuncCode;
    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);

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
uint8_t* newWriteMultipleRegs(uint16_t startingAddress, uint16_t quantity, uint16_t* data, int* len) {
    *len = quantity * 2;  // 2 bytes for starting address + 2 bytes for quantity
    uint8_t* pdu = (uint8_t*)malloc(*len);
    if (pdu == NULL) {
        MALLOC_ERR;
        return NULL;
    }

    pdu[0] = (uint8_t)writeMultipleRegsFuncCode;
    pdu[1] = (uint8_t)startingAddress >> 8;
    pdu[2] = (uint8_t)startingAddress & 0xFF;
    pdu[3] = (uint8_t)quantity >> 8;
    pdu[4] = (uint8_t)quantity & 0xFF;
    pdu[5] = (uint8_t)quantity * 2;

    // set pdu request data in Big Endian format
    for (int i = 0; i < quantity; i++) {
        pdu[2 * i + 5] = (uint8_t)data[i] >> 8;    // high byte
        pdu[2 * i + 6] = (uint8_t)data[i] & 0xFF;  // low byte
    }

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

    int len, sent, id;
    uint8_t* packet = newWriteMultipleRegs(startingAddress, quantity, data, &len);
    if (packet == NULL) {
        return -1;
    }
    id = writeMultipleRegsFuncCode;
    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);

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

#undef MALLOC_ERR