#include "applicationLayer/modbusApp.h"

#include "log.h"

char* getFunctionCodeString(uint8_t fCode) {
    switch (fCode) {
        case readHoldingRegsFuncCode:
            return "Read Holding Registers";
        case writeMultipleRegsFuncCode:
            return "Write Multiple Registers";
        default:
            return "not implemented";
    }
}

sds mbapHeaderToString(modbusMBAP mbapHeader) {
    char mbapHeaderBuffer[MODBUS_MBAP_HEADER_SIZE];
    mbapHeaderBuffer[0] = mbapHeader.transactionIdentifier >> 8;
    mbapHeaderBuffer[1] = mbapHeader.transactionIdentifier & 0xFF;
    mbapHeaderBuffer[2] = mbapHeader.protocolIdentifier >> 8;
    mbapHeaderBuffer[3] = mbapHeader.protocolIdentifier & 0xFF;
    mbapHeaderBuffer[4] = mbapHeader.length >> 8;
    mbapHeaderBuffer[5] = mbapHeader.length & 0xFF;
    mbapHeaderBuffer[6] = mbapHeader.unitIdentifier;

    return sdsnewlen(mbapHeaderBuffer, MODBUS_MBAP_HEADER_SIZE);
}

sds flatenPacketToString(modbusPacketTCP packet) {
    sds flattened = mbapHeaderToString(packet.mbapHeader);
    flattened = sdscatlen(flattened, &packet.pdu.fCode, sizeof(packet.pdu.fCode));
    flattened = sdscatlen(flattened, packet.pdu.data, packet.pdu.byteCount);

    return flattened;
}

void logModbusPacket(modbusPacketTCP packet) {
    LOG("MBAP Header: %02X %02X %02X %02X %02X %02X %02X %02X\n",
        packet.mbapHeader.transactionIdentifier >> 8, packet.mbapHeader.transactionIdentifier & 0xFF,
        packet.mbapHeader.protocolIdentifier >> 8, packet.mbapHeader.protocolIdentifier & 0xFF,
        packet.mbapHeader.length >> 8, packet.mbapHeader.length & 0xFF,
        packet.mbapHeader.unitIdentifier >> 8, packet.mbapHeader.unitIdentifier & 0xFF);
}

int sendModbusRequest(modbusPacketTCP request, int socketfd) {
    // flatten request to a string compatible with send()
    sds requestString = flatenPacketToString(request);

    // send request and free memory
    int result = sendModbusRequestTCP(socketfd, requestString, sdslen(requestString));
    sdsfree(requestString);

    return result;
}

int receiveModbusResponse(int socketfd, uint16_t transactionID, modbusPacketTCP* response) {
    // receive response
    char responseBuffer[MODBUS_ADU_MAX_SIZE];

    if (receiveModbusResponseTCP(socketfd, responseBuffer, MODBUS_ADU_MAX_SIZE) < 0) {
        ERROR("failed to receive response\n");
        return -1;
    }

    // parse mbap header
    response->mbapHeader.transactionIdentifier = (responseBuffer[0] << 8) | responseBuffer[1];
    response->mbapHeader.protocolIdentifier = (responseBuffer[2] << 8) | responseBuffer[3];
    response->mbapHeader.length = (responseBuffer[4] << 8) | responseBuffer[5];
    response->mbapHeader.unitIdentifier = responseBuffer[6];

    // check transaction ID
    if (response->mbapHeader.transactionIdentifier != transactionID) {
        ERROR("transaction ID mismatch\n");
        return -1;
    }

    // parse pdu
    response->pdu.fCode = responseBuffer[7];
    response->pdu.byteCount = responseBuffer[8];
    for (int i = 0; i < response->pdu.byteCount; i++) {
        response->pdu.data[i] = responseBuffer[9 + i];
    }

    return 0;
}

// read holding registers and return the response
// return -1 if error, 0 if success
int readHoldingRegisters(int socketfd, uint16_t startingAddress, uint16_t quantity) {
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

    // build request
    modbusPacketTCP message;

    // set mbap header
    message.mbapHeader.transactionIdentifier = 0x0001;
    message.mbapHeader.protocolIdentifier = 0;  // modbus protocol
    message.mbapHeader.length = 0x0006;         // 7 bytes after mbap header
    message.mbapHeader.unitIdentifier = 0x01;   // unit identifier

    // set pdu
    message.pdu.fCode = readHoldingRegsFuncCode;

    // set pdu request data in Big Endian format
    message.pdu.data[0] = startingAddress >> 8;
    message.pdu.data[1] = startingAddress & 0xFF;
    message.pdu.data[2] = quantity >> 8;
    message.pdu.data[3] = quantity & 0xFF;

    // set byte count
    message.pdu.byteCount = 4;

    // log request mbap header
    logModbusPacket(message);

    // print response fcode and enum string representation
    printf("Response fcode: %02X -> %s\n", message.pdu.fCode, getFunctionCodeString(message.pdu.fCode));
    
    // print request
    printf("Request data: ");
    for (int i = 0; i < message.pdu.byteCount; i++) {
        printf("%02X ", message.pdu.data[i]);
    }
    printf("\n");

    // send request
    if (sendModbusRequest(message, socketfd) < 0) {
        ERROR("failed to send request\n");
        return -1;
    }

    // receive response as a data string
    modbusPacketTCP response;
    if (receiveModbusResponse(socketfd, message.mbapHeader.transactionIdentifier, &response) < 0) {
        ERROR("failed to receive response\n");
        return -1;
    }

    // log response mbap header
    logModbusPacket(response);

    // print response fcode and enum string representation
    printf("Response fcode: %02X -> %s\n", response.pdu.fCode, getFunctionCodeString(response.pdu.fCode));
    
    // print response
    printf("Response data: ");
    for (int i = 0; i < response.pdu.byteCount; i++) {
        printf("%02X ", response.pdu.data[i]);
    }
    printf("\n");

    return 0;
}