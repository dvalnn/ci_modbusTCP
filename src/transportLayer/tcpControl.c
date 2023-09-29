#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "linkLayer/tcpControl.h"

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
    if (socketfd < 0)
        return -1;

    // set timeout
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
        return -2;

    // set keepalive
    int optval = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
        return -2;

    return socketfd;
}

/**
 * @brief close a TCP socket
 *
 * @param socketd socket file descriptor
 * @return 0 if success, -1 if error
 */
int tcpCloseSocket(int socketfd) {
    return close(socketfd);
}

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
int tcpConnect(int socketfd, char* ip, int port) {
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // convert IPv4 addresses from text to binary form
    if (inet_aton(ip, &server.sin_addr) == 0)
        return -2;

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

    int n = 0;
    while (received < pLen) {
        n = recv(socket, packet + received, pLen - received, 0);
        if (n < 0) {
            return -1;
        }
        received += n;
    }

    return received;
}