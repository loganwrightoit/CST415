#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionUtil.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define MAX_BUFSIZE 144

WSADATA wsaData;
struct sockaddr_in clientService;

bool putReq(SOCKET sock, char * inStr)
{
    return xmt(sock, inStr);
}

bool getReq(SOCKET sock, char * outStr)
{
    return rcv(sock, outStr);
}

bool putRsp(SOCKET sock, char * inStr)
{
    return xmt(sock, inStr);
}

bool getRsp(SOCKET sock, char * outStr)
{
    return rcv(sock, outStr);
}

//
// Shuts down connection and socket and returns array of results.
// First parameter is receive close status, second is send close status, third is socket close status.
//
int* shutdownSocket(SOCKET sock)
{
    int * results = new int[3];

    results[0] = shutdown(sock, SD_RECEIVE); // Close receiving side
    if (results[0] == SOCKET_ERROR) {
        printf("ERROR: Socket receive shutdown failed: %d\n", WSAGetLastError());
    }
    results[1] = shutdown(sock, SD_SEND); // Close sending side
    if (results[1] == SOCKET_ERROR) {
        printf("ERROR: Socket send shutdown failed: %d\n", WSAGetLastError());
    }

    results[2] = closesocket(sock);
    WSACleanup();

    return results;
}

//
// Transmit message over connection.
// Returns true if message successfully sent.
//
bool xmt(SOCKET sock, CHAR * inStr)
{
	string str = "";

    /*
    Field Name: TCPHeader
    Data type Binary (Network, Big Endian Bit Order)
    Justification: Right
    Length: 2
    Begin position: 0
    End position 1
    Description: Binary value that represents the length of the request message,
    excluding these two bytes.
    */

    int inSize = strlen(inStr);

    // Prepend message with two-byte length header
	BYTE bytes[2] = { (inSize & 0xff00) >> 8, inSize & 0xff };
	str.append((const char*)bytes, 2);

    // Append data
	str.append(inStr);

	char * toSend = new char[strlen(inStr) + 2];
	memcpy(toSend, str.c_str(), str.length());

	int remaining = str.length();
    int total = 0;
    while (remaining > 0)
    {
		int result = send(sock, toSend + total, min(remaining, MAX_BUFSIZE), 0);
        if (result == SOCKET_ERROR)
        {
            delete[] toSend;
            return false;
        }

        total += result;
        remaining -= result;
    }

	delete[] toSend;
    return true;
}

//
// Receive transaction data and return string.
//
bool rcv(SOCKET sock, char * outStr)
{
    u_short messageLength;
    int inBytes = recv(sock, (char*)&messageLength, sizeof(messageLength), 0);

	if (inBytes > 0)
	{
		messageLength = ntohs(messageLength);
		int total = 0;
		while (total < messageLength)
		{
			int size = min((int)(messageLength - total), MAX_BUFSIZE);
			int result = recv(sock, (char*)(outStr + total), size, 0);
			total += result;
		}

		return true;
	}

    return false;
}

//
// Initializes Winsock
//
bool initWinsock()
{
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Winsock 2.2
    if (result != NO_ERROR) {
        cout << "ERROR starting winsock: " << result << endl;
        return false;
    }

    return true;
}

//
// Sets blocking mode of socket.
// TRUE blocks, FALSE is non-blocking
//
void setBlocking(SOCKET sock, bool block)
{
    block = !block;
    unsigned long mode = block;
    int result = ioctlsocket(sock, FIONBIO, &mode);
    if (result != NO_ERROR)
    {
        cout << "ERORR failed setting socket blocking mode: " << result << endl;
    }
}

//
// Opens a client connection to server.
//  Params:
//      host - IP address of server
//      port - Port of server
//      timeoutSec - Timeout for connection attempt in seconds
//  Return:
//      a SOCKET pointer
//
SOCKET connectTo(char * host, u_short port, int timeoutSec)
{
    TIMEVAL Timeout;
    Timeout.tv_sec = timeoutSec;
    Timeout.tv_usec = 0;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in address;
    address.sin_addr.s_addr = inet_addr(host);
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    // Set to non-blocking and start a connection attempt

    setBlocking(sock, false);
    connect(sock, (struct sockaddr *)&address, sizeof(address));

    fd_set Write, Err;
    FD_ZERO(&Write);
    FD_ZERO(&Err);
    FD_SET(sock, &Write);
    FD_SET(sock, &Err);

    // Set back to blocking and check if socket is ready after timeout

    setBlocking(sock, true);
    select(0, NULL, &Write, &Err, &Timeout);
    if (FD_ISSET(sock, &Write))
    {
        setBlocking(sock, false);
        return sock;
    }
    else {
        return INVALID_SOCKET;
    }
}