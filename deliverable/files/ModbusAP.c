#include "ModbusAP.h"

#include <stdio.h>
#include <stdlib.h>

#include "ModbusTCP.h"

// comment this line to disable debug messages
// #define DEBUG

#ifdef DEBUG

#define ERROR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

#else

#define ERROR(...)
#define MALLOC_ERR

#endif

/**
 * @brief Connect to the server
 *
 * @param ip server ip string
 * @param port server port
 *
 * @return socket file descriptor, -1 if error
 */
int connectToServer(char* ip, int port) {
    return modbusConnect(ip, port, TIMEOUT_SEC, TIMEOUT_USEC);
}

/**
 * @brief Disconnect from the server
 *
 * @param socketfd socket file descriptor
 */
void disconnectFromServer(int socketfd) {
    modbusDisconnect(socketfd);
}

/**
 * @brief Create a Read Holding Registers request
 *
 * @param startingAddress starting address of the registers to read
 * @param quantity number of registers to read
 * @param len pointer to the length of the request
 * @return uint8_t* pointer to the request created -- must be freed by the caller
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
uint8_t* readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t id, uint16_t quantity, int* rlen) {
    if (socketfd < 0) {
        ERROR("invalid socket\n");
        return NULL;
    }

    if (quantity < MODBUS_QUANTITY_MIN || quantity > MODBUS_RHR_QUANTITY_MAX) {
        ERROR("quantity must be between %d and %d\n", MODBUS_QUANTITY_MIN, MODBUS_RHR_QUANTITY_MAX);
        return NULL;
    }

    if (startingAddress < MODBUS_ADDRESS_MIN || startingAddress > MODBUS_ADDRESS_MAX) {
        ERROR("starting address must be between %d and %d\n", MODBUS_ADDRESS_MIN, MODBUS_ADDRESS_MAX);
        return NULL;
    }

    if (startingAddress + quantity > MODBUS_ADDRESS_MAX) {
        ERROR("starting address + quantity must be less than %d\n", MODBUS_ADDRESS_MAX);
        return NULL;
    }

    int len, sent;
    uint8_t* packet = newReadHoldingRegs(startingAddress, quantity, &len);
    if (packet == NULL) {
        return NULL;
    }

    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);

    if (len != sent) {
        ERROR("failed to send Write Multiple Registers request\n\tlen: %d sent %d\n",
              len, sent);
        return NULL;
    }

    packet = modbusReceive(socketfd, id, &len);

    if (packet == NULL) {
        ERROR("failed to receive Read Holding Registers response\n");
        return NULL;
    }

    *rlen = len;
    return packet;
}

/**
 * @brief Create a Write Multiple Registers request
 *
 * @param startingAddress starting address of the registers to write
 * @param quantity number of registers to write
 * @param data pointer to the data to write
 * @param len pointer to the length of the request
 * @return uint8_t* pointer to the request created -- must be freed by the caller
 */
uint8_t* newWriteMultipleRegs(uint16_t startingAddress, uint16_t quantity, uint16_t* data, int* len) {
    *len = quantity * 2 + 6;  // 2 bytes for starting address + 2 bytes for quantity
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
    pdu[5] = (uint8_t)quantity * 2;  // number of bytes to follow

    // set pdu request data in Big Endian format
    for (int i = 0; i < quantity; i++) {
        pdu[2 * i + 6] = (uint8_t)data[i] >> 8;    // high byte
        pdu[2 * i + 7] = (uint8_t)data[i] & 0xFF;  // low byte
    }

    return pdu;
}

/**
 * @brief send a Write Multiple Registers request to the server
 *
 * @param socketfd socket file descriptor
 * @param startingAddress starting address of the registers to write
 * @param quantity quantity of registers to write
 * @param data pointer to the data to write
 * @param rlen pointer to the response length
 * @return uint8_t* pointer to the response buffer -- must be freed by the caller
 */
uint8_t* writeMultipleRegisters(int socketfd, uint16_t startingAddress, uint16_t id, uint16_t quantity, uint16_t* data, int* rlen) {
    if (socketfd < 0) {
        ERROR("invalid socket\n");
        return NULL;
    }

    if (quantity < MODBUS_QUANTITY_MIN || quantity > MODBUS_WMR_QUANTITY_MAX) {
        ERROR("quantity must be between %d and %d\n", MODBUS_QUANTITY_MIN, MODBUS_WMR_QUANTITY_MAX);
        return NULL;
    }

    if (startingAddress < MODBUS_ADDRESS_MIN || startingAddress > MODBUS_ADDRESS_MAX) {
        ERROR("starting address must be between %d and %d\n", MODBUS_ADDRESS_MIN, MODBUS_ADDRESS_MAX);
        return NULL;
    }

    if (startingAddress + quantity > MODBUS_ADDRESS_MAX) {
        ERROR("starting address + quantity must be less than %d\n", MODBUS_ADDRESS_MAX);
        return NULL;
    }

    int len, sent;
    uint8_t* packet = newWriteMultipleRegs(startingAddress, quantity, data, &len);
    if (packet == NULL) {
        return NULL;
    }

    sent = modbusSend(socketfd, id, packet, len);

    // free memory
    free(packet);

    if (len != sent) {
        ERROR("failed to send Write Multiple Registers request\n\tlen: %d sent %d\n",
              len, sent);
        return NULL;
    }

    packet = modbusReceive(socketfd, id, &len);

    if (packet == NULL) {
        ERROR("failed to receive Write Multiple Registers response\n");
        return NULL;
    }

    *rlen = len;
    return packet;
}

#undef MALLOC_ERR