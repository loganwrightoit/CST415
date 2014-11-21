#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <atlstr.h>
#include <thread>
#include "ConnectionUtil.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#define MAX_MSG_SIZE 144

char serverIp[16] = "";
int serverPort = 0;
int hostPort = 0;

// Forward declarations
SOCKET GetListenSock();
SOCKET server_socket = INVALID_SOCKET;
SOCKET client_socket = INVALID_SOCKET;
unsigned __stdcall ClientSession(void *data);
unsigned __stdcall ServerSession(void *data);
void ServerRspToClientRsp(char * rsp);

//
// Main program thread.
//
int main(int argc, char* argv[])
{
    bool validArgs = true;

    if (argc > 2)
    {
        for (int arg = 1; arg < argc && validArgs; ++arg)
        {
            if (strcmp(argv[arg], "-serverIp") == 0)
            {
                struct sockaddr_in sock_addr;
                if (inet_pton(AF_INET, argv[++arg], &sock_addr) > 0)
                {
                    strcpy_s(serverIp, argv[arg]);
                }
                else
                {
                    validArgs = false;
                }
            }
            else if (strcmp(argv[arg], "-serverPort") == 0)
            {
                serverPort = atoi(argv[++arg]);
            }
            else if (strcmp(argv[arg], "-hostPort") == 0)
            {
                hostPort = atoi(argv[++arg]);
            }
            else
            {
                validArgs = false;
            }
        }
    }
    else
    {
        validArgs = false;
    }

    if (!validArgs)
    {
        cout << "ERROR: arguments required: -serverIp <ip> -serverPort <port> -hostPort <port>" << endl;
        return 1;
    }

    if (!initWinsock())
    {
        cout << "ERROR: Winsock initialization failed: " << WSAGetLastError() << endl;
        return 1;
    }

    SOCKET listen_socket = GetListenSock();
    if (listen_socket == INVALID_SOCKET)
    {
        cout << "ERROR: GetListenSock() failed: " << WSAGetLastError() << endl;
        return 1;
    }

    // Grab local IP
    char szBuffer[1024];
    gethostname(szBuffer, sizeof(szBuffer));
    struct hostent *host = gethostbyname(szBuffer);
    struct in_addr addr;
    memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));

    cout << "Middleware Server started successfully." << endl;
    cout << "IP: " << inet_ntoa(addr) << endl;
    cout << "Port: " << hostPort << endl;

    cout << "Connecting to server..." << endl;

    server_socket = connectTo(serverIp, serverPort, 3);
    if (server_socket == INVALID_SOCKET)
    {
        cout << "ERROR: Unable to connect to remote server, shutting down..." << endl;
        return 1;
    }
    setBlocking(server_socket, true);

    // Create server transaction thread
    unsigned threadID;
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ServerSession, NULL, 0, &threadID);

    // Run until closed
    while (1)
    {
        if (server_socket == INVALID_SOCKET)
            break;

        if (client_socket == INVALID_SOCKET)
        {
            client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket != INVALID_SOCKET)
            {
                unsigned threadID;
                HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)client_socket, 0, &threadID);
            }
        }
    }

    return 0;
}

//
// Returns listening socket.
//
SOCKET GetListenSock()
{
    SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        cout << "ERROR: GetListenSock() socket() failed with error: " << WSAGetLastError() << endl;
        return sock;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(hostPort);

    if (::bind(sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        cout << "ERROR: GetListenSock() bind() failed with error: " << WSAGetLastError() << endl;
        closesocket(sock);
        return sock;
    }
    if (listen(sock, 5) == SOCKET_ERROR)
    {
        cout << "ERROR: GetListenSock() listen() failed with error: " << WSAGetLastError() << endl;
        closesocket(sock);
        return sock;
    }

    return sock;
}

unsigned __stdcall ClientSession(void *data)
{
    SOCKET sock = (SOCKET)data;
    int sockNum = sock;

    cout << "Client connected on socket " << sock << endl;

    while (1)
    {
        if (sock == INVALID_SOCKET)
            break;

        // Get request message
        char req[MAX_MSG_SIZE] = "";

        // Blocking recv
        if (!getReq(sock, req))
            break;

        // Forward request to server (watch for bottleneck in thread)
        if (!putReq(server_socket, req))
            break;
    }
    
    cout << "Client disconnected on socket " << sockNum << endl;

    return 1;
}

unsigned __stdcall ServerSession(void *data)
{
    int sockNum = server_socket;
    cout << "Server connected on socket " << sockNum << endl;
    cout << "Waiting for client connection..." << endl;

    while (1)
    {
        if (server_socket == INVALID_SOCKET)
            break;

        // Get response message
        char rsp[MAX_MSG_SIZE] = "";

        // Blocking recv
        if (!getRsp(server_socket, rsp))
            break;

        // Change host info to match server
        ServerRspToClientRsp(rsp);

        // Send response back to client
        if (!putRsp(client_socket, rsp))
            break;
    }

    cout << "Server disconnected on socket " << sockNum << endl;

    return 1;
}

//
// Transforms response message by replacing host info with server info.
//
void ServerRspToClientRsp(char * rsp)
{
    // Decode message fields
    char * pch;
    pch = strtok(rsp, "|");
    pch = strtok(NULL, "|");
    long msTimeStamp = atol(pch);           // msTimeStamp
    pch = strtok(NULL, "|");
    int requestId = atoi(pch);              // RequestID
    pch = strtok(NULL, "|");
    char studentName[21] = "";              // StudentName
    strcpy(studentName, pch);
    pch = strtok(NULL, "|");
    char studentId[8] = "";                 // StudentID
    strcpy(studentId, pch);
    pch = strtok(NULL, "|");
    int responseDelay = atoi(pch);          // ResponseDelay
    pch = strtok(NULL, "|");
    char foreignHostIpAddress[16] = "";     // ClientIPAddress
    strcpy(foreignHostIpAddress, pch);
    pch = strtok(NULL, "|");
    int foreignHostServicePort = atoi(pch); // ClientServicePort
    pch = strtok(NULL, "|");
    int serverSocketNumber = atoi(pch);     // ClientSocketNumber
    pch = strtok(NULL, "|");
    char serverIpAddress[22] = "";          // ServerIPAddress
    strcpy(serverIpAddress, pch);
    pch = strtok(NULL, "|");
    int serverServicePort = atoi(pch);      // ServerServicePort   - TRANSFORM
    pch = strtok(NULL, "|");
    char responseId[21] = "";               // ResponseID (data)
    strcpy(responseId, pch);
    pch = strtok(NULL, "|");
    int responseType = atoi(pch);           // ResponseType

    // Rebuild response so middleware is transparent to client
    string msg = "";
    msg.append("RSP");
    msg.append("|");
    msg.append(std::to_string(msTimeStamp)); // DONE
    msg.append("|");
    msg.append(std::to_string(requestId)); // DONE
    msg.append("|");
    msg.append(studentName); // DONE
    msg.append("|");
    msg.append(studentId); // DONE
    msg.append("|");
    msg.append(std::to_string(responseDelay)); // DONE
    msg.append("|");
    
    char addr[INET_ADDRSTRLEN] = "NULL";
    struct sockaddr_in sin;
    int addrlen = sizeof(sin);
    getpeername(client_socket, (struct sockaddr *)&sin, &addrlen);
    int local_port = ntohs(sin.sin_port);

    inet_ntop(AF_INET, &sin.sin_addr, addr, sizeof(addr));
    
    //msg.append(foreignHostIpAddress); // INSERT CLIENT_SOCKET IP
    msg.append(addr);
    msg.append("|");
    //msg.append(std::to_string(foreignHostServicePort)); // INSERT CLIENT_SOCKET PORT
    msg.append(std::to_string(local_port));

    msg.append("|");
    msg.append(std::to_string(serverSocketNumber)); // DONE
    msg.append("|");
    msg.append(serverIpAddress); // DONE - INSERT TRUE SERVER IP
    msg.append("|");
    msg.append(std::to_string(serverServicePort)); // DONE - INSERT TRUE SERVER PORT
    msg.append("|");
    msg.append(responseId);
    msg.append("|");
    msg.append(std::to_string(responseType));
    msg.append("|");

    // Replace original string
    strcpy(rsp, const_cast<char*>(msg.c_str()));
}