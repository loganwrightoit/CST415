#include <iostream>
#include <ws2tcpip.h>
#include "ConnectionModule.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 256

WSADATA wsaData;
struct sockaddr_in clientService;

void putReq(SOCKET sock, char * inStr)
{
     xmt(sock, inStr);
}

void getReq(SOCKET sock, char * outStr)
{
	rcv(sock, outStr);
}

void putRsp(SOCKET sock, char * inStr)
{
    xmt(sock, inStr);
}

void getRsp(SOCKET sock, char * outStr)
{
	rcv(sock, outStr);
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

    // Prepend message with two-byte length header

    u_short len = strlen(inStr);
    byte byteLen[] = { (len & 0xff00) >> 8, (len & 0xff) };
    string s = "";
    s += byteLen[0];
    s += byteLen[1];
    s += inStr;

	// Transmit message

    int result = send(sock, s.c_str(), s.length(), 0);
    if (result == SOCKET_ERROR) {
        cout << "DEBUG: Error requesting: " << WSAGetLastError() << ", shutting down connection." << endl;
        shutdownSocket(sock);
    }
    else {
        //cout << inStr << endl;
        return true;
    }

    return false;
}

//
// Receive transaction data and return string.
//
void rcv(SOCKET sock, char * outStr)
{
    string s = "";

    // Receive first two bytes (message length)
    u_short messageLength;
    int inBytes = recv(sock, (char*)&messageLength, sizeof(messageLength), 0);

    if (inBytes > 0) // Receive all the data
    {
        int bytesLeft = inBytes;
        do {
            // Receive all the data
            char buffer[DEFAULT_BUFLEN] = "";
            bytesLeft -= recv(sock, (char*)&buffer, sizeof(buffer), 0);
            s.append(buffer);
        } while (bytesLeft > 0);
    }
    else if (inBytes == WSAEWOULDBLOCK) // Nothing to receive, don't block
    {
		strcpy(outStr, const_cast<char*>(s.c_str()));
    }
    else if (inBytes == 0) // Shutdown successful
    {
		strcpy(outStr, const_cast<char*>(s.c_str()));
    }
    else // An error occurred
    {
        cout << "DEBUG: Error receiving: " << WSAGetLastError() << ", shutting down connection." << endl;
        shutdownSocket(sock);
    }

	strcpy(outStr, const_cast<char*>(s.c_str()));
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

    /*
    SOCKET sock;
    int result;

    cout << "SOCKET: creating socket..." << endl;

    // Create a SOCKET for connecting to server
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cout << "ERROR creating socket: " << WSAGetLastError() << endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Set socket to non-blocking
    setNonBlocking(sock);

    cout << "SOCKET: connecting to server..." << endl;

    // The sockaddr_in structure specifies the address family, IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(addr);
    clientService.sin_port = htons(port);

    // Attempt to connect to server, with timeout after 3 seconds
    long sysTime = GetTickCount();
    do {
        int result = connect(sock, (SOCKADDR*)&clientService, sizeof(clientService));
        if (result == SOCKET_ERROR) {
            cout << "ERROR connecting to server: " << WSAGetLastError() << endl;
            closesocket(sock);
            WSACleanup();
            return INVALID_SOCKET;
        }
    } while ();

    return sock;
    */
}