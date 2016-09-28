#ifndef __IPC_H__
#define __IPC_H__

int sendPacket(int sock, char* msg);
int recvResponse(int sock, char** dataReceived, int* receivedLen);
int attachToHost(char* ip, int port);
int detach(int sock);

#endif

