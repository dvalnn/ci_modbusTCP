#include "../include/modbusApp.h"

int sendModbusRequest(modbusTcpPacket request, int socketfd) {
    // flatten request to a string compatible with send()
    char mbapHeaderString[MODBUS_MBAP_HEADER_SIZE];
    mbapHeaderString[0] = request.mbapHeader.transactionIdentifier >> 8;
    mbapHeaderString[1] = request.mbapHeader.transactionIdentifier & 0xFF;
    mbapHeaderString[2] = request.mbapHeader.protocolIdentifier >> 8;
    mbapHeaderString[3] = request.mbapHeader.protocolIdentifier & 0xFF;
    mbapHeaderString[4] = request.mbapHeader.length >> 8;
    mbapHeaderString[5] = request.mbapHeader.length & 0xFF;
    mbapHeaderString[6] = request.mbapHeader.unitIdentifier;

    sds requestString = sdsempty();
    requestString = sdscatlen(requestString, mbapHeaderString, MODBUS_MBAP_HEADER_SIZE);
    requestString = sdscatlen(requestString, &request.pdu.fCode, sizeof(request.pdu.fCode));
    requestString = sdscatlen(requestString, &request.pdu.data, sizeof(request.pdu.byteCount));

    // send request
    return sendModbusRequestTCP(socketfd, requestString, sdslen(requestString));
}

int receiveModbusResponse(int socketfd, uint16_t transactionID, sds data) {
    // receive response
    char responseBuffer[MODBUS_ADU_MAX_SIZE];

    if (receiveModbusResponseTCP(socketfd, responseBuffer, MODBUS_ADU_MAX_SIZE) < 0) {
        fprintf(stderr, "Error: failed to receive response\n");
        return -1;
    }

    modbusTcpPacket response;

    // parse mbap header
    response.mbapHeader.transactionIdentifier = (responseBuffer[0] << 8) | responseBuffer[1];
    response.mbapHeader.protocolIdentifier = (responseBuffer[2] << 8) | responseBuffer[3];
    response.mbapHeader.length = (responseBuffer[4] << 8) | responseBuffer[5];
    response.mbapHeader.unitIdentifier = responseBuffer[6];

    // check transaction ID
    if (response.mbapHeader.transactionIdentifier != transactionID) {
        fprintf(stderr, "Error: transaction ID mismatch\n");
        return -1;
    }

    // parse pdu
    response.pdu.fCode = responseBuffer[7];
    response.pdu.byteCount = responseBuffer[8];
    for (int i = 0; i < response.pdu.byteCount; i++) {
        response.pdu.data[i] = responseBuffer[9 + i];
    }

    // copy data to sds
    data = sdscatlen(data, &response.pdu.data, response.pdu.byteCount);

    return 0;
}

// read holding registers and return the response
// return -1 if error, 0 if success
int readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity) {
    if (quantity < MODBUS_REG_QUANTITY_MIN || quantity > MODBUS_REG_QUANTITY_MAX) {
        fprintf(stderr, "Error: quantity must be between %d and %d\n", MODBUS_REG_QUANTITY_MIN,
                MODBUS_REG_QUANTITY_MAX);
        return -1;
    }

    if (startingAddress < MODBUS_ADDRESS_MIN || startingAddress > MODBUS_ADDRESS_MAX) {
        fprintf(stderr, "Error: starting address must be between %d and %d\n", MODBUS_ADDRESS_MIN,
                MODBUS_ADDRESS_MAX);
        return -1;
    }

    if (startingAddress + quantity > MODBUS_ADDRESS_MAX) {
        fprintf(stderr, "Error: starting address + quantity must be less than %d\n", MODBUS_ADDRESS_MAX);
        return -1;
    }

    // build request
    modbusTcpPacket message;
    message.pdu.fCode = readHoldingRegsFuncCode;

    // build request data in Big Endian
    message.pdu.data[0] = startingAddress >> 8;
    message.pdu.data[1] = startingAddress & 0xFF;
    message.pdu.data[2] = quantity >> 8;
    message.pdu.data[3] = quantity & 0xFF;

    // set byte count
    message.pdu.byteCount = 4;

    // set mbap header
    message.mbapHeader.transactionIdentifier = 0x0000;
    message.mbapHeader.protocolIdentifier = 0;  // modbus protocol
    message.mbapHeader.length = 0x0006;         // 6 bytes
    message.mbapHeader.unitIdentifier = 0x01;   // unit identifier

    // send request
    if (sendModbusRequest(message, socketfd) < 0) {
        fprintf(stderr, "Error: failed to send request\n");
        return -1;
    }

    // receive response as a data string
    sds response = sdsempty();
    if (receiveModbusResponse(socketfd, message.mbapHeader.transactionIdentifier, response) < 0) {
        fprintf(stderr, "Error: failed to receive response\n");
        return -1;
    }

    // print response and free memory
    printf("Response: %s\n", response);
    sdsfree(response);

    return 0;
}