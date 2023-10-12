#include "ModbusTCP.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// comment this line to disable debug messages
// #define DEBUG

#ifdef DEBUG

#define INFO(...) fprintf(stdout, "[INFO] " __VA_ARGS__)
#define ERROR(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
#define MALLOC_ERR \
    ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

#else

#define INFO(...)
#define ERROR(...)
#define MALLOC_ERR

#endif

//*****************************************************************************
//******************************  TCP CONTROL *********************************
//*****************************************************************************
/**
 * @brief create a TCP socket and set timeout and keepalive options
 *
 * @param seconds timeout seconds
 * @param microseconds timeout microseconds
 * @return socket file descriptor if success,
 *         -1 if error creating the socket,
 *         -2 if error setting the options
 */
int tcpOpenSocket(time_t seconds, suseconds_t microseconds) {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) return -1;

    // set timeout
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                   sizeof(timeout)) < 0)
        return -2;

    // set keepalive
    int optval = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval,
                   sizeof(optval)) < 0)
        return -2;

    return socketfd;
}

/**
 * @brief close a TCP socket
 *
 * @param socketd socket file descriptor
 * @return 0 if success, -1 if error
 */
int tcpCloseSocket(int socketfd) { return close(socketfd); }

/**
 * @brief connect to a TCP server
 *
 * @param socketfd socket file descriptor
 * @param ip server IP address
 * @param port server port
 * @return 0 if success,
 *        -1 if a connection error occurs,
 *        -2 if the IP address is invalid
 */
int tcpConnect(int socketfd, char* ipString, int port) {
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // convert IPv4 addresses from text to binary form
    if (inet_aton(ipString, &server.sin_addr) == 0) return -2;

    return connect(socketfd, (struct sockaddr*)&server, sizeof(server));
}

/**
 * @brief send a packet through a TCP socket
 *
 * @param socketfd socket file descriptor
 * @param packet packet to send
 * @param pLen packet length
 * @return n bytes sent if success, -1 if error
 */
int tcpSend(int socketfd, uint8_t* packet, int pLen) {
    int sent = 0;
    int n = 0;
    while (sent < pLen) {
        n = send(socketfd, packet + sent, pLen - sent, 0);
        if (n < 0) {
            return -1;
        }
        sent += n;
    }
    return sent;
}

/**
 * @brief receive a packet through a TCP socket
 *
 * @param socketfd socket file descriptor
 * @param packet packet to receive
 * @param pLen packet length
 * @return n bytes received if success, -1 if error
 */
int tcpReceive(int socketfd, uint8_t* packet, int pLen) {
    int received = 0;

    received = recv(socketfd, packet, pLen, 0);
    if (received < 0) {
        ERROR("invalid packet\n");
        return -1;
    }

    return received;
}

//*****************************************************************************
//****************************** DATA PACKAGING *******************************
//*****************************************************************************
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

#define UNIT_ID 51
#define PROTOCOL_ID 0

#define MODBUS_MBAP_HEADER_SIZE 7

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
ModbusADU* _newModbusADU(uint16_t transactionID, uint16_t protocolIdentifier,
                         uint8_t unitIdentifier, uint8_t* pdu, int pduLen) {
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
        ERROR(
            "newModbusADU: invalid parameters\n\ttransactionID: %d, pdu: %p, "
            "pduLen: %d\n",
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
    if (adu == NULL) return;

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
    mbapHeader[6] = adu->unitIdentifier;

    // pdu length = adu->length - unit identifier
    int pduLen = adu->length - 1;

    // concatenate the MBAP header and the PDU
    // the MBAP header is not included in the length field
    uint8_t* packet =
        (uint8_t*)malloc((MODBUS_MBAP_HEADER_SIZE + pduLen) * sizeof(*packet));
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

//*****************************************************************************
//*********************************  MODBUS TCP *******************************
//*****************************************************************************

/**
 * @brief send a modbus Request
 *
 * @param socketfd socket file descriptor
 * @param id transaction identifier
 * @param pdu protocol data unit
 * @param pLen protocol data unit length
 * @return int sent bytes if success (< 0 if error)
 */
int modbusSend(int socketfd, uint16_t id, uint8_t* pdu, int pLen) {
    ModbusADU* adu = newModbusADU(id, pdu, pLen);
    if (adu == NULL) {
        return -1;
    }
    int sent = sendModbusADU(socketfd, adu);
    freeModbusADU(adu);
    return sent;
}

/**
 * @brief receive a modbus Response
 *
 * @param socketfd socket file descriptor
 * @param pdu pointer to the pdu buffer (must be freed by the caller)
 * @return int pdu length if success, -1 if error, -2 id mismatch
 */
uint8_t* modbusReceive(int socketfd, uint16_t id, int* pduLen) {
    ModbusADU* adu = receiveModbusADU(socketfd);
    if (adu == NULL) {
        return NULL;
    }

    if (adu->unitIdentifier != UNIT_ID) {
        freeModbusADU(adu);
        ERROR("unit identifier mismatch\n\treceived: %d\n\texpected: %d\n",
              adu->unitIdentifier, UNIT_ID);
        return NULL;
    }

    if (adu->transactionID != id) {
        freeModbusADU(adu);
        ERROR("transaction id mismatch\n\treceived: %d\n\texpected: %d\n",
              adu->transactionID, id);
        return NULL;
    }

    *pduLen = adu->length - 1;
    uint8_t* pdu = (uint8_t*)malloc(*pduLen);
    if (pdu == NULL) {
        MALLOC_ERR;
        freeModbusADU(adu);
        return NULL;
    }

    memcpy(pdu, adu->pdu, *pduLen);
    freeModbusADU(adu);

    return pdu;
}

/**
 * @brief connect top a modbus server through TCP
 *
 * @param ip server IP address
 * @param port server port - 502 for modbus
 * @param seconds connection timeout seconds
 * @param microseconds connection timeout microseconds
 * @return socket file descriptor if success, -1 if error
 */
int modbusConnect(char* ip, int port, time_t seconds,
                  suseconds_t microseconds) {
    int socketfd = tcpOpenSocket(seconds, microseconds);
    if (socketfd < 0) {
        ERROR("Cannot opening tcp socket: \n\tError code: %d\n", socketfd);
        return -1;
    }

    int err = tcpConnect(socketfd, ip, port);
    if (err < 0) {
        ERROR("Cannot connect to modbus tcp server: \n\tError code: %d\n", err);
        return -1;
    }

    return socketfd;
}

/**
 * @brief disconnect from a modbus server, previously connected through TCP
 *
 * @param socketfd socket file descriptor
 * @return 0 if success, -1 if error
 */
int modbusDisconnect(int socketfd) { return tcpCloseSocket(socketfd); }
