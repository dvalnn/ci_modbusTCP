#include <stdio.h>
#include <stdlib.h>

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

sds pduToString(modbusPDU pdu) {
    sds pduString = sdsnewlen(&pdu.fCode, sizeof(pdu.fCode));
    pduString = sdscatlen(pduString, pdu.data, pdu.byteCount);

    return pduString;
}

sds flatenPacketToString(modbusPacket packet) {
    sds flattened = mbapHeaderToString(packet.mbapHeader);
    flattened = sdscatlen(flattened, &packet.pdu.fCode, sizeof(packet.pdu.fCode));
    flattened = sdscatlen(flattened, packet.pdu.data, packet.pdu.byteCount);

    return flattened;
}

void logPacket(modbusPacket packet) {
    INFO("transaction ID: %02X\n", packet.mbapHeader.transactionIdentifier);
    INFO("protocol ID: %02X\n", packet.mbapHeader.protocolIdentifier);
    INFO("length: %02X\n", packet.mbapHeader.length);
    INFO("unit ID: %02X\n", packet.mbapHeader.unitIdentifier);
    INFO("function code: %02X\n", packet.pdu.fCode);
    INFO("byte count: %02X\n", packet.pdu.byteCount);
    INFO("data: ");
    for (int i = 0; i < packet.pdu.byteCount; i++) {
        INFO("%02X ", packet.pdu.data[i]);
    }
    INFO("\n\n");
}

int sendModbusRequest(modbusPacket request, int socketfd) {
    // flatten request to a string compatible with send()
    sds requestString = flatenPacketToString(request);

    // send request and free memory
    int result = sendModbusRequestTCP(socketfd, requestString, sdslen(requestString));
    sdsfree(requestString);

    return result;
}

// free a modbusPacketTCP struct instance
void freeModbusPacketTCP(modbusPacket* packet) {
    free(packet->pdu.data);
    free(packet);
}

// dinamically alocate a new modbusPacketTCP struct instance and return it
modbusPacket* newModbusPacketTCP(uint16_t dataLength) {
    modbusPacket* packet = (modbusPacket*)malloc(sizeof(modbusPacket));
    if (packet == NULL) {
        ERROR("failed to allocate memory for modbusPacketTCP\n");
        return NULL;
    }

    packet->mbapHeader.transactionIdentifier = 0;
    packet->mbapHeader.protocolIdentifier = 0;
    packet->mbapHeader.length = 0;
    packet->mbapHeader.unitIdentifier = 0;

    packet->pdu.fCode = 0;
    packet->pdu.byteCount = dataLength;

    packet->pdu.data = (uint8_t*)malloc(dataLength * sizeof(packet->pdu.data[0]));

    for (int i = 0; i < dataLength; i++) {
        packet->pdu.data[i] = 0;
    }

    return packet;
}

// create a new modbusPacket with a read holding registers request
// return NULL if error
modbusPacket* newReadHoldingRegs(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t transactionID) {
    int dataLength = 4;  // 2 bytes for starting address + 2 bytes for quantity

    // build request
    modbusPacket* message = newModbusPacketTCP(dataLength);
    message->pdu.byteCount = dataLength;
    // set mbap header
    message->mbapHeader.transactionIdentifier = transactionID;
    message->mbapHeader.protocolIdentifier = 0;  // modbus protocol
    message->mbapHeader.length = 0x0006;         // 7 bytes after mbap header
    message->mbapHeader.unitIdentifier = 0x01;   // unit identifier

    // set pdu
    message->pdu.fCode = readHoldingRegsFuncCode;

    // set pdu request data in Big Endian format
    message->pdu.data[0] = startingAddress >> 8;
    message->pdu.data[1] = startingAddress & 0xFF;
    message->pdu.data[2] = quantity >> 8;
    message->pdu.data[3] = quantity & 0xFF;

    // set byte count

    return message;
}

// create a new modbusPacket with a write multiple registers request
// return NULL if error
modbusPacket* newWriteMultipleRegs(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data, uint16_t transactionID) {
    int dataLength = 2 * quantity;  // 2 bytes for each data

    // build request
    modbusPacket* message = newModbusPacketTCP(dataLength);
    if (message == NULL)
        return NULL;

    // set mbap header
    message->mbapHeader.transactionIdentifier = 0x0001;
    message->mbapHeader.protocolIdentifier = 0;  // modbus protocol
    message->mbapHeader.unitIdentifier = 0x01;   // unit identifier

    // set pdu
    message->pdu.fCode = writeMultipleRegsFuncCode;
    message->pdu.byteCount = dataLength;

    // set pdu request data in Big Endian format
    for (int i = 0; i < quantity; i++) {
        message->pdu.data[2 * i] = data[i] >> 8;        // high byte
        message->pdu.data[2 * i + 1] = data[i] & 0xFF;  // low byte
    }

    // 7 bytes after mbap header + 1 byte for fCode + byteCount bytes for data
    message->mbapHeader.length = 0x0006 + 1 + message->pdu.byteCount;

    return message;
}

int sendReadHoldingRegs(int socketfd, uint16_t startingAddress, uint16_t quantity) {
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

    // int id = newTransactionID();
    uint16_t id = readHoldingRegsFuncCode;
    modbusPacket* message = newReadHoldingRegs(socketfd, startingAddress, quantity, id);
    if (message == NULL) {
        return -1;
    }

    INFO("\nRead Holding Registers request\n");
    logPacket(*message);

    // print packet as hex
    if (sendModbusRequest(*message, socketfd) < 0) {
        ERROR("failed to send Read Holding Registers request\n");
        freeModbusPacketTCP(message);
        return -1;
    }

    // free memory
    freeModbusPacketTCP(message);
    return id;
}

int sendWriteMultipleRegs(int socketfd, uint16_t startingAddress, uint16_t quantity, uint16_t* data) {
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

    // int id = newTransactionID();
    uint16_t id = writeMultipleRegsFuncCode;
    modbusPacket* message = newWriteMultipleRegs(socketfd, startingAddress, quantity, data, id);
    if (message == NULL) {
        return -1;
    }

    INFO("\nWrite Multiple Registers request\n");
    logPacket(*message);

    // send request
    if (sendModbusRequest(*message, socketfd) < 0) {
        ERROR("failed to send Write Multiple Registers request\n");
        freeModbusPacketTCP(message);
        return -1;
    }

    // free memory
    freeModbusPacketTCP(message);
    return id;
}

int receiveReadHoldingRegisters(modbusPacket* packet, char* buffer) {
    packet->pdu.fCode = buffer[7];
    if (packet->pdu.fCode == 0x83) {
        ERROR("error code: %02X -> Read Holding Registers\n", buffer[8]);
        packet->pdu.byteCount = 1;
        packet->pdu.data[0] = buffer[8];  // error byte
        return 0;
    }

    if (packet->pdu.fCode != readHoldingRegsFuncCode) {
        ERROR("Read Holding Registers: function code mismatch: %02X\n", packet->pdu.fCode);
        packet->pdu.byteCount = 0;
        return 1;
    }

    packet->pdu.byteCount = buffer[8];
    for (int i = 0; i < packet->pdu.byteCount; i++) {
        packet->pdu.data[i] = buffer[9 + i];
    }

    return 0;
}

int receiveWriteMultipleRegisters(modbusPacket* packet, char* buffer) {
    packet->pdu.fCode = buffer[7];
    if (packet->pdu.fCode == 0x90) {
        ERROR("error code: %02X -> Write Multiple Registers\n", buffer[8]);
        packet->pdu.byteCount = 1;
        packet->pdu.data[0] = buffer[8];  // error byte
        return 0;
    }

    if (packet->pdu.fCode != writeMultipleRegsFuncCode) {
        ERROR("Write Multiple Registers: function code mismatch: %02X\n", packet->pdu.fCode);
        packet->pdu.byteCount = 0;
        return 1;
    }

    packet->pdu.byteCount = 4;
    for (int i = 0; i < packet->pdu.byteCount; i++) {
        packet->pdu.data[i] = buffer[8 + i];
    }

    return 0;
}

sds receiveReply(int socketfd, uint16_t transactionID, uint8_t fCode) {
    // receive response
    char responseBuffer[MODBUS_ADU_MAX_SIZE];

    if (receiveModbusResponseTCP(socketfd, responseBuffer, MODBUS_ADU_MAX_SIZE) < 0)
        return NULL;

    modbusPacket* response = newModbusPacketTCP(MODBUS_DATA_MAX_SIZE);

    // parse mbap header
    response->mbapHeader.transactionIdentifier = (responseBuffer[0] << 8) | responseBuffer[1];
    response->mbapHeader.protocolIdentifier = (responseBuffer[2] << 8) | responseBuffer[3];
    response->mbapHeader.length = (responseBuffer[4] << 8) | responseBuffer[5];
    response->mbapHeader.unitIdentifier = responseBuffer[6];

    // check transaction ID
    if (response->mbapHeader.transactionIdentifier != transactionID) {
        ERROR("transaction ID mismatch\n");
        freeModbusPacketTCP(response);
        return NULL;
    }

    int error;
    switch (fCode) {
        case readHoldingRegsFuncCode:
            error = receiveReadHoldingRegisters(response, responseBuffer);
            break;
        case writeMultipleRegsFuncCode:
            error = receiveWriteMultipleRegisters(response, responseBuffer);
            break;
        default:
            ERROR("function code not implemented\n");
            freeModbusPacketTCP(response);
            return NULL;
    }

    if (error) {
        freeModbusPacketTCP(response);
        return NULL;
    }

    INFO("packet received\n");
    logPacket(*response);

    // parse pdu data to string in hex
    sds dataString = sdsnewlen(NULL, response->pdu.byteCount / 2);
    for (int i = 0; i < response->pdu.byteCount / 2; i++) {
        dataString[i] = ((response->pdu.data[2 * i] << 8) | response->pdu.data[2 * i + 1]);
    }

    // free memory
    freeModbusPacketTCP(response);

    return dataString;
}