#include "../include/modbusTCP.h"

// open a TCP socket
// return the socket if success, -1 if error
int openTCPSocket() {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
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
    inet_aton(ip, &server.sin_addr);

    if (connect(socketfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        return -1;
    }

    return 0;
}

// connect to the modbus server through TCP
int connectToModbusTCP(char* ip, int port) {
    int socketfd = openTCPSocket();
    if (socketfd < 0) {
        printf("Cannot open socket\n");
        return -1;
    }

    if (connectToServer(socketfd, ip, port) < 0) {
        printf("Cannot connect to server\n");
        return -1;
    }

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
int receiveModbusResponseTCP(int socket, char* response, int responseLength) {
    int received = 0;
    int n = 0;
    while (received < responseLength) {
        n = recv(socket, response + received, responseLength - received, 0);
        if (n < 0) {
            return -1;
        }
        received += n;
    }
    return 0;
}