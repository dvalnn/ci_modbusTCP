#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "modbusTCP.h"

#include "log.h"

// create a non-blocking TCP socket
int openTCPSocket() {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        ERROR("cannot open socket\n");
        return -1;
    }

    // set timeout
    struct timeval timeout;
    timeout.tv_sec = MODBUS_TIMEOUT_SEC;
    timeout.tv_usec = MODBUS_TIMEOUT_USEC;

    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        ERROR("cannot set socket timeout\n");
        return -1;
    }

    return socketfd;
}

// close a TCP socket
int closeTCPSocket(int socketfd) {
    return close(socketfd);
}

// connect to the server
// return 0 if success, -1 if error
int connectToServer(int socketfd, char* ip, int port) {
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    printf("Connecting to %s:%d\n", ip, port);

    // convert IPv4 addresses from text to binary form
    if (inet_aton(ip, &server.sin_addr) == 0) {
        ERROR("invalid IP address\n");
        return -1;
    }

    return connect(socketfd, (struct sockaddr*)&server, sizeof(server));
}

// connect to the modbus server through TCP
int connectToModbusTCP(char* ip, int port) {
    int socketfd = openTCPSocket();
    if (socketfd < 0) {
        ERROR("cannot open socket\n");
        return -1;
    }

    if (connectToServer(socketfd, ip, port) < 0) {
        closeTCPSocket(socketfd);
        ERROR("cannot connect to server\n");
        return -1;
    }

    printf("Connected to %s:%d successfully\n", ip, port);
    return socketfd;
}

// disconnect from the modbus server through TCP
int disconnectFromModbusTCP(int socketfd) {
    return closeTCPSocket(socketfd);
}

// send a modbus request through TCP
// return 0 if success, -1 if error
int sendModbusRequestTCP(int socket, char* request, int requestLength) {
    int sent = 0;
    int n = 0;
    while (sent < requestLength) {
        n = send(socket, request + sent, requestLength - sent, 0);
        if (n < 0) {
            return -1;
        }
        sent += n;
    }
    return 0;
}

// receive a modbus response through TCP
// return 0 if success, -1 if error
// timeout the receive call after 1 second
int receiveModbusResponseTCP(int socket, char* response, int responseLength) {
    int received = 0;

    // int n = 0;
    // while (received < responseLength) {
    //     n = recv(socket, response + received, responseLength - received, 0);
    //     if (n < 0) {
    //         return -1;
    //     }
    //     received += n;
    // }

    received = recv(socket, response, responseLength, 0);
    if (received < 0) {
        ERROR("Server response timeout\n");
        return -1;
    }

    return 0;
}