#pragma once

bool xmt(SOCKET sock, const char * request);
const char * rcv(SOCKET sock);
bool putReq(SOCKET sock, const char * request);
const char * getReq(SOCKET sock);
bool putRsp(SOCKET sock, const char * response);
const char * getRsp(SOCKET sock);

bool initWinsock();
bool setNonBlocking(SOCKET sock);
int* shutdownSocket(SOCKET sock);
SOCKET connectTo(char * host, u_short port, int timeout);
bool disconnectFrom(SOCKET s);