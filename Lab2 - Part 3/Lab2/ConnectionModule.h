#pragma once

bool xmt(SOCKET sock, char * inStr);
bool rcv(SOCKET sock, char * outStr);
void putReq(SOCKET sock, char * inStr);
bool getReq(SOCKET sock, char * outStr);
void putRsp(SOCKET sock, char * inStr);
bool getRsp(SOCKET sock, char * outStr);

bool initWinsock();
void setBlocking(SOCKET sock, bool block);
int* shutdownSocket(SOCKET sock);
SOCKET connectTo(char * host, u_short port, int timeoutSec);
bool disconnectFrom(SOCKET sock);