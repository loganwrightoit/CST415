#include "WinSock2.h"

bool xmt(SOCKET sock, char * inStr);
void rcv(SOCKET sock, char * outStr);
void putReq(SOCKET sock, char * inStr);
void getReq(SOCKET sock, char * outStr);
void putRsp(SOCKET sock, char * inStr);
void getRsp(SOCKET sock, char * outStr);

bool initWinsock();
void setBlocking(SOCKET sock, bool block);
int* shutdownSocket(SOCKET sock);
SOCKET connectTo(char * host, u_short port, int timeoutSec);
bool disconnectFrom(SOCKET sock);