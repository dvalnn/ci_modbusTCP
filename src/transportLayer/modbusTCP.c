#include "transportLayer/modbusTCP.h"

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

#include "log.h"
#include "sds.h"
#include "sdsalloc.h"
#include "transportLayer/dataPackaging.h"
#include "transportLayer/tcpControl.h"

#define MALLOC_ERR ERROR("malloc failed in %s: %s, %d\n", __func__, __FILE__, __LINE__)

/**
 * @brief send a modbus Request
 *
 * @param socketfd socket file descriptor
 * @param id transaction identifier
 * @param pdu protocol data unit
 * @param pLen protocol data unit length
 * @return int sent bytes if success (< 0 if error)
 */
int modbusSend(int socketfd, uint8_t id, uint8_t* pdu, int pLen) {
    modbusADU* adu = newModbusADU(id, pdu, pLen);
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
int modbusReceive(int socketfd, uint8_t id, uint8_t* pdu) {
    modbusADU* adu = receiveModbusADU(socketfd);
    if (adu == NULL) {
        return -1;
    }

    if (adu->transactionID != id) {
        freeModbusADU(adu);
        return -2;
    }

    int pduLen = adu->length - 1;
    pdu = (uint8_t*)malloc(pduLen);
    if (pdu == NULL) {
        MALLOC_ERR;
        freeModbusADU(adu);
        return -1;
    }

    memcpy(pdu, adu->pdu, pduLen);
    freeModbusADU(adu);
    return pduLen;
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
int modbusConnect(char* ip, int port, time_t seconds, suseconds_t microseconds) {
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
int modbusDisconnect(int socketfd) {
    return tcpCloseSocket(socketfd);
}

#undef MALLOC_ERR