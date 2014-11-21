#pragma comment (lib, "Ws2_32.lib")

#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <atlstr.h>
#include "ConnectionUtil.h"

using namespace std;

#define MAX_MSG_SIZE 144
const std::string delimiter("|");

char ipAddr[16] = "";
int portNum = 0;
int rspId = 0;

// Forward declarations
SOCKET GetListenSock();
unsigned __stdcall ClientSession(void *data);
string getRspFromReq(SOCKET sock, char * inStr);

//
// Main program thread.
//
int main(int argc, char* argv[])
{
    bool validArgs = true;

    if (argc == 3)
    {
        for (int arg = 1; arg < argc && validArgs; ++arg)
        {
            if (strcmp(argv[arg], "-port") == 0)
            {
                portNum = atoi(argv[++arg]);
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
        cout << "ERROR: arguments required: -port <port number>" << endl;
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

    cout << "Server started successfully." << endl;
    cout << "IP: " << inet_ntoa(addr) << endl;
    cout << "Port: " << portNum << endl;
    cout << "Listening for connections..." << endl;
        
    while (1)
    {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket != INVALID_SOCKET)
        {
            unsigned threadID;
            HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, (void*)client_socket, 0, &threadID);
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
        cout << "ERROR: socket() failed with error: " << WSAGetLastError() << endl;
        return sock;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(portNum);

    if (::bind(sock, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        cout << "ERROR: bind() failed with error: " << WSAGetLastError() << endl;
        closesocket(sock);
        return sock;
    }
    if (listen(sock, 5) == SOCKET_ERROR)
    {
        cout << "ERROR: listen() failed with error: " << WSAGetLastError() << endl;
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

        if (!getReq(sock, req))
            break;

        // Generate response message      
        string rsp = getRspFromReq(sock, req);

        // Send response
        if (!putRsp(sock, const_cast<char*>(rsp.c_str())))
            break;
    }

    cout << "Client disconnected on socket " << sockNum << endl;

    return 1;
}

//
// Returns response string to request.
//
string getRspFromReq(SOCKET sock, char * inStr)
{
    string req(inStr);
    size_t pos = req.find(delimiter);
    req.erase(0, pos + delimiter.length());

    // Fields
    long msTimestamp = 0;
    int requestId = 0;
    string studentName;
    string studentId;
    int responseDelay = 0;
    string foreignHostIpAddress;
    int foreignHostServicePort = 0;
    int serverSocketNumber = 0;
    string serverIpAddress;
    int serverServicePort = 0;
    string responseId;
    int responseType = 0;

    // Tokenizer
    for (int numToken = 0; numToken < 12; ++numToken)
    {
        size_t pos = req.find(delimiter);
        string token = req.substr(0, pos);

        switch (numToken)
        {
            case 0:
                msTimestamp = atol(token.c_str());
                break;
            case 1:
                requestId = atoi(token.c_str());
                break;
            case 2:
                studentName = token;
                break;
            case 3:
                studentId = token;
                break;
            case 4:
                responseDelay = atoi(token.c_str());
                break;
            case 5:
                foreignHostIpAddress = token;
                break;
            case 6:
                foreignHostServicePort = atoi(token.c_str());
                break;
            case 7:
                serverSocketNumber = atoi(token.c_str());
                break;
            case 8:
                serverIpAddress = token;
                break;
            case 9:
                serverServicePort = atoi(token.c_str());
                break;
            case 10:
                responseId = token;
                break;
            case 11:
                // Response Type (only for lab 2 clients)
                responseType = atoi(token.c_str());
                break;
        }

        req.erase(0, pos + delimiter.length());
    }

    string result;

    /*
    Field Name: MessageType
    Data type: ASCII Alphanumeric fixed “RSP”
    Justification: Any
    Length: 3
    Begin position: 2
    End position: 4
    Description: This will be a fixed three-character sequence of “RSP”, all in
    upper case.
    */

    result.append("RSP");
    result.append(delimiter);

    /*
    Field Name: msTimeStamp
    Data type: ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 10
    Begin position: 6
    End position: Variable
    Description: A millisecond time stamp value from a running elapsed system
    timer on the server
    */

    DWORD timestamp = GetTickCount();
    result.append(std::to_string(timestamp));
    result.append(delimiter);

    /*
    Field Name: RequestID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Data from the RequestID field of the client’s request message
    */

    result.append(std::to_string(requestId));
    result.append(delimiter);

    /*
    Field Name: StudentName
    Data type: ASCII Text
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Data from the StudentName field of the client’s request message
    */

    result.append(studentName);
    result.append(delimiter);

    /*
    Field Name: Student ID
    Data type: ASCII Aphanumeric
    Justification: Any
    Length: Variable with a maximum length of 7
    Begin position: Variable
    End position: Variable
    Description: Data from the StudentID field of the client’s request message
    */

    result.append(studentId);
    result.append(delimiter);

    /*
    Field Name: ResponseDelay
    Data type: ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: Data from the ResponseDelay field of the client’s request
    message
    */

    result.append(std::to_string(responseDelay));
    result.append(delimiter);

    /*
    Field Name: ForeignHostIPAddress
    Data type: ASCII Alphanumeric
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the server’s foreign host in dotted decimal
    notation
    */

    result.append(foreignHostIpAddress);
    result.append(delimiter);

    /*
    Field Name: ForeignHostServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server’s foreign host’s service port number, should always be
    2605.
    */

    result.append(std::to_string(foreignHostServicePort));
    result.append(delimiter);

    /*
    Field Name: ServerSocketNumber
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server’s TCP socket number.
    */

    result.append(std::to_string(sock));
    result.append(delimiter);

    /*
    Field Name: ServerIPAddress
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the server in dotted decimal notation.
    */

    char addr[INET_ADDRSTRLEN] = "NULL";
    struct sockaddr_in sin;
    int addrlen = sizeof(sin);
    getsockname(sock, (struct sockaddr *)&sin, &addrlen);
    int local_port = ntohs(sin.sin_port);

    inet_ntop(AF_INET, &sin.sin_addr, addr, sizeof(addr));
    result.append(addr);
    result.append(delimiter);

    /*
    Field Name: ServerServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server’s service port number, should be 2605.
    */

    result.append(std::to_string(local_port));
    result.append(delimiter);

    /*
    Field Name: ResponseID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: A unique value generated by the server to exclusively identify
    responses.
    */

    if (++rspId > 99999)
    {
        rspId = 0;
    }

    result.append("RSP_ID_");
    result.append(std::to_string(rspId));
    result.append(delimiter);

    /*
    Field Name: ResponseType
    Data type: ASCII Numeric
    Justification: Any
    Length: 1
    Begin position: Variable
    End position Variable
    Description: This response field is to be populated by the client to indicate the
    following:
    1. Normal response received from server
    2. Stand-in response generated by client
    3. Latent response received from server
    4. Spurious or unsolicited response received from server
    */

    result.append("1");
    result.append(delimiter);

    /*
    Maximum response length = 14610
    Maximum binary value in TCP header field = 14410, or HEX 0090
    */

    return result;
}