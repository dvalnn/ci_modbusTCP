#ifndef _TCP_CONTROL_H_
#define _TCP_CONTROL_H_

int tcpCloseSocket(int socketfd);
int tcpOpenSocket(time_t seconds, suseconds_t microseconds);

int tcpConnect(int socketfd, char* ip, int port);

int tcpSend(int socketfd, uint8_t* packet, int pLen);
int tcpReceive(int socketfd, uint8_t* packet, int pLen);

#endif  // _TCP_CONTROL_H_