#pragma once
#include "WinSock2.h"

bool xmt(SOCKET sock, char * inStr);
bool rcv(SOCKET sock, char * outStr);
bool putReq(SOCKET sock, char * inStr);
bool getReq(SOCKET sock, char * outStr);
bool putRsp(SOCKET sock, char * inStr);
bool getRsp(SOCKET sock, char * outStr);

bool initWinsock();
void setBlocking(SOCKET sock, bool block);
int* shutdownSocket(SOCKET sock);
SOCKET connectTo(char * host, u_short port, int timeoutSec);
bool disconnectFrom(SOCKET sock);