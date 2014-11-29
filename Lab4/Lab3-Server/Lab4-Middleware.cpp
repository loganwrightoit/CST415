#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <atlstr.h>
#include <thread>
#include <map>
#include "ConnectionUtil.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

#define MAX_MSG_SIZE 144
const std::string delimiter("|");

char serverIp[16] = "";
int serverPort = 0;
int hostPort = 0;
bool hasClient = false;
bool endProgram = false;

// Fields for transaction stats
int numTx = 0;
int msDelay = 0;
char serverName[21] = "";
int expectedTx = 0;
LARGE_INTEGER frequency;
u_long firstReqTime = 0;
u_long firstRspTime = 0;
u_long finalReqTime = 0;
u_long finalRspTime = 0;

// Fields used for building RSP messages quickly
char mdw_addr[INET_ADDRSTRLEN] = "NULL";
int mdw_port = 0;
char srv_addr[INET_ADDRSTRLEN] = "NULL";
int srv_port = 0;

thread serverThread;

// Stores request messages for middleware client log output
map<size_t, char*> reqMsgs;

// Forward declarations
SOCKET GetListenSock();
SOCKET server_socket = INVALID_SOCKET;
SOCKET client_socket = INVALID_SOCKET;
unsigned __stdcall ClientSession(void *data);
unsigned __stdcall ServerSession(void *data);
void doReqToClientReq(char * inStr);
void addTrailRecord(int shutTransmit, int shutReceive, int shutSocket);

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
        if (endProgram)
        {
            break;
        }

        if (server_socket == INVALID_SOCKET)
            return 0;

        if (client_socket == INVALID_SOCKET)
        {
            client_socket = accept(listen_socket, NULL, NULL);
            if (client_socket != INVALID_SOCKET)
            {
                hasClient = true;
                unsigned threadID;
                HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &ClientSession, 0, 0, &threadID);
            }
        }
    }

    int* shutdownStatus = shutdownSocket(server_socket);
    addTrailRecord(shutdownStatus[0], shutdownStatus[1], shutdownStatus[2]);
    delete[] shutdownStatus;

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
    cout << "Client connected on socket " << client_socket << ", beginning log output." << endl;

    freopen("LabX.ScenarioX.WrightL.txt", "w", stdout); // Output to file

    while (1)
    {
        if (client_socket == INVALID_SOCKET)
            break;

        // Get request message
        char req[MAX_MSG_SIZE] = "";

        // Blocking recv
        if (!getReq(client_socket, req))
            break;

        // Stats
        if (firstReqTime == 0) firstReqTime = GetTickCount();
        finalReqTime = GetTickCount();

        // Insert our information into new request message, and save for output later
        doReqToClientReq(req);

        // Send our request to server
        if (!putReq(server_socket, req))
            break;
    }

    // Track final request timestamp
    finalReqTime = GetTickCount();
    
    endProgram = true;

    return 1;
}

unsigned __stdcall ServerSession(void *data)
{
    // Gather MIDDLEWARE CLIENT socket information for use later
    struct sockaddr_in mdw_sin;
    int mdw_addrlen = sizeof(mdw_sin);
    getsockname(server_socket, (struct sockaddr *)&mdw_sin, &mdw_addrlen);
    mdw_port = ntohs(mdw_sin.sin_port);
    inet_ntop(AF_INET, &mdw_sin.sin_addr, mdw_addr, sizeof(mdw_addr)); // Fill addr string with client IP

    // Gather SERVER socket information for use later
    struct sockaddr_in srv_sin;
    int srv_addrlen = sizeof(srv_sin);
    getpeername(server_socket, (struct sockaddr *)&srv_sin, &srv_addrlen);
    srv_port = ntohs(srv_sin.sin_port);
    inet_ntop(AF_INET, &srv_sin.sin_addr, srv_addr, sizeof(srv_addr)); // Fill ADDR string with SERVER IP

    int sockNum = server_socket;
    cout << "Server connected on socket " << sockNum << endl;
    cout << "Waiting for client connection..." << endl;
    
    // Spin until client connects
    while (1)
    {
        if (hasClient) break;
        Sleep(0);
    }

    // Populate frequency information for collecting transaction stats
    QueryPerformanceFrequency(&frequency);

    // Receive until client closes connection or error occurs
    while (1)
    {
        // Get response message
        char rsp[MAX_MSG_SIZE] = "";

        // Blocking recv
        if (!getRsp(server_socket, rsp))
            break;

        // Stats
        if (firstRspTime == 0) firstRspTime = GetTickCount();
        finalRspTime = GetTickCount();

        // Grab REQ ID
        string temp(rsp);
        size_t pos = temp.find(delimiter);
        temp.erase(0, pos + delimiter.length()); // Trim REQ from message
        pos = temp.find(delimiter);
        temp.erase(0, pos + delimiter.length()); // Trim msTimestamp from message
        pos = temp.find(delimiter);
        string token(temp.substr(0, pos)); // Grab requestId

        // Match RSP to REQ ID
        hash<string> fn_hash;
        auto element = reqMsgs.find(fn_hash(token));
        if (element != reqMsgs.end())
        {
            // Output to log
            cout << element->second << endl;
            cout << rsp << endl;

            // Increment transaction number for trailer log output
            ++numTx;

            // Cleanup request memory in cache
            delete[] element->second;
        }

        // Forward response to client
        if (!putRsp(client_socket, rsp))
            break;
    }

    return 1;
}

//
// Creates a new REQ by substituting middleware information into original REQ
// and replacing the string.
//
void doReqToClientReq(char * inStr)
{
    // Log response as last always
    finalRspTime = GetTickCount();

    string req(inStr);

    // Trim REQ from string
    size_t pos = req.find(delimiter);
    req.erase(0, pos + delimiter.length());

    // Fields
    long msTimestamp = 0;
    string requestId;
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

    // Tokenize remainder of string
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
            requestId = token;
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

    // Transform response and forward back to client
    string msg;
    msg.append("REQ");
    msg.append("|");
    msg.append(std::to_string(msTimestamp));
    msg.append("|");
    msg.append(requestId);
    msg.append("|");
    msg.append(studentName);
    msg.append("|");
    msg.append(studentId);
    msg.append("|");
    msg.append(std::to_string(responseDelay));
    msg.append("|");
    msg.append(mdw_addr); // Replace with MIDDLEWARE ADDR
    msg.append("|");
    msg.append(std::to_string(mdw_port)); // Replace with MIDDLEWARE PORT
    msg.append("|");
    msg.append(std::to_string(server_socket)); // Replace with SERVER socket handle
    msg.append("|");
    msg.append(srv_addr); // Replace with SERVER ADDR
    msg.append("|");
    msg.append(std::to_string(srv_port)); // Replace with SERVER PORT
    msg.append("|");
    msg.append(responseId);
    msg.append("|");
    msg.append(std::to_string(responseType));
    msg.append("|");

    // Save REQ for output later
    hash<string> fn_hash;
    char * temp = new char[MAX_MSG_SIZE];
    strcpy(temp, msg.c_str());
    reqMsgs.insert(std::pair<size_t, char*>(fn_hash(requestId), temp));

    // Replace original string
    strcpy(inStr, const_cast<char*>(msg.c_str()));
}

//
// Outputs trail record.
//
void addTrailRecord(int shutTransmit, int shutReceive, int shutSocket)
{
    string msg = "";

    std::time_t rawtime;
    std::tm* timeinfo;
    char buffer[80];
    std::time(&rawtime);
    timeinfo = std::localtime(&rawtime);

    /*
    Field Name : Date
    Data type : ASCII, in the form of MMDDYYYY
    Justification : Any
    Length : 8
    Begin position : 0
    End position: 7
    Description:  Date of completion of lab scenario
    */

    std::strftime(buffer, 80, "%m%d%Y", timeinfo);
    msg.append(buffer);
    msg.append("|");

    /*
    Field Name:  Time
    Data type:  ASCII, in the form of HHMMSS [24 hr. notation]
    Justification: Any
    Length:  6
    Begin position:  9
    End position: 14
    Description:  Time of completion of lab scenario
    */

    std::strftime(buffer, 80, "%H%M%S", timeinfo);
    msg.append(buffer);
    msg.append("|");

    /*
    Field Name:  RcvShutdownStatus
    Data type:  ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length:  5
    Begin position:  16
    End position: 20
    Description:  Return status from issuing a shutdown of the receive half session.
    Insert a vertical ‘|’ character at position 21
    */

    msg.append(std::to_string(shutTransmit));
    msg.append("|");

    /*
    Field Name:  XmtShutdownStatus
    Data type:  ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length:  5
    Begin position:  22
    End position: 26
    Description:  Return status from issuing a shutdown of the transmit half
    session.
    */

    msg.append(std::to_string(shutReceive));
    msg.append("|");

    /*
    Field Name : CloseStatus
    Data type : ASCII Numeric
    Justification : Right, space or zero filled to the left
    Length : 5
    Begin position : 28
    End position : 32
    Description : Return status from issuing a socket close command
    */

    msg.append(std::to_string(shutSocket));

    cout << msg << endl;

    double reqRunDuration = (finalReqTime - firstReqTime);
    double rspRunDuration = (finalRspTime - firstRspTime);
    double txRunDuration = (finalRspTime - firstReqTime);

    //Requests transmitted = [xxxxx]
    cout << "Requests Transmitted   = [" << numTx << "]" << endl;
    //Responses received = [xxxxx]
    cout << "Responses Received     = [" << numTx << "]" << endl;
    //Req.run duration(ms) = [xxxxxxxxx]
    cout << "Req. run duration (ms) = [" << setprecision(2) << fixed << reqRunDuration << "]" << endl;
    //Rsp.Run duration(ms) = [xxxxxxxxx]
    cout << "Rsp. run duration (ms) = [" << setprecision(2) << fixed << rspRunDuration << "]" << endl;
    //Trans.Duration(ms) = [xxxxxxxxx]
    cout << "Trans. duration   (ms) = [" << setprecision(2) << fixed << txRunDuration << "]" << endl;
    //Actual req.pace(ms) = [xxxx]
    cout << "Actual req. pace  (ms) = [" << setprecision(2) << fixed << (reqRunDuration / numTx) << "]" << endl;
    //Actual rsp.Pace(ms) = [xxxx]
    cout << "Actual rsp. pace  (ms) = [" << setprecision(2) << fixed << (rspRunDuration / numTx) << "]" << endl;
    //Configured pace(ms) = [xxxx]
    cout << "Configured pace   (ms) = [" << msDelay << "]" << endl;
    //Transaction avg. (ms) = [xxxx]
    cout << "Transaction avg.  (ms) = [" << setprecision(2) << fixed << (txRunDuration / numTx) << "]" << endl;
    //Client Name:
    cout << "Client student: WrightL" << endl;
    //Server Name:
    cout << "Server student: " << serverName << endl;
}