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

#define DEFAULT_PORT 2605
#define MAX_BUFSIZE 144

bool isServer = false;
char server_IP[16];
int numRequests = 0;
int requestID = 0;

int genRequestID()
{
    return requestID++;
}

string getMessage(SOCKET sock)
{
    string msg = "";
    int reqId = genRequestID();

    /*
    Field Name: MessageType
    Data type: ASCII Alphanumeric fixed �REQ�
    Justification: Any
    Length: 3
    Begin position: 2
    End position: 4
    Description: This is to be a fixed three-character sequence of �REQ�, all in
    upper case.
    */

    msg.append("REQ");
    msg.append("|");

    /*
    Field Name: msTimeStamp
    Data type: ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 10
    Begin position: 6
    End position: Variable
    Description: A millisecond time stamp value from a running elapsed system timer on the client
    */

    // This isn't working.. time fluctuates down and up

    DWORD timestamp = GetTickCount();
    msg.append(std::to_string(timestamp));
    msg.append("|");

    /*
    Field Name: RequestID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: A unique value among all request messages that is used to
    exclusively match a response message to it respective request
    message. This field will be returned by the server in the response
    message and in the same positions.
    */

    msg.append(std::to_string(reqId));
    msg.append("|");

    /*
    Field Name: StudentName
    Data type: ASCII Text
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Student�s last name followed by first letter of student�s first name.
    for example: �FindleyT� [without quotation marks, upper case
    is optional].
    */

    msg.append("WrightL");
    msg.append("|");

    /*
    Field Name: StudentID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length: Variable with a maximum length of 7
    Begin position: Variable
    End position: Variable
    Description: Student�s �918� ID number in the form of �dd-dddd�
    Field Name: ResponseDelay
    Data type: ASCII Numeric
    */

    msg.append("19-4928");
    msg.append("|");

    /*
    !!!!  MISSING FIRST TWO INSTRUCTIONS FOR THIS FIELD !!!
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: A numeric value that represents the number of milliseconds the
    server is to delay before sending a response.  Maximum of 3000 ms.
    */

    msg.append("0");
    msg.append("|");

    /*
    Field Name: ClientIPAddress
    Data type: ASCII Alphanumeric
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the client in dotted decimal notation
    */

    char addr[INET_ADDRSTRLEN] = "NULL";
    struct sockaddr_in sin;
    int addrlen = sizeof(sin);
    getsockname(sock, (struct sockaddr *)&sin, &addrlen);
    int local_port = ntohs(sin.sin_port);

    inet_ntop(AF_INET, &sin.sin_addr, addr, sizeof(addr));

    msg.append(addr); // Need to add clientside IP address
    msg.append("|");

    /*
    Field Name: ClientServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The client�s service port number that is being used in the TCP
    session with the instructor�s server.
    */

    msg.append(std::to_string(local_port));
    msg.append("|");

    /*
    Field Name: ClientSocketNumber
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The client�s TCP socket number that is being used in the TCP
    session with the instructor�s server.
    */

    msg.append(std::to_string(sock));
    msg.append("|");

    /*
    Field Name: ForeignHostIPAddress
    Data type: ASCII Alphanumeric
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the foreign host (server) in dotted decimal
    notation
    */

    msg.append(server_IP); // Need to add server IP address
    msg.append("|");

    /*
    Field Name: ForeignHostServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The foreign host�s (server�s) service port number that is being
    used in the TCP session with the student�s client (should always
    be 2605).
    */

    msg.append(std::to_string(DEFAULT_PORT));
    msg.append("|");

    /*
    Field Name: StudentData
    Data type ASCII Alphanumeric
    Justification: Any
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Discretionary data defined by the student, may be all spaces. Will
    not be returned in the response.
    */

    msg.append("This is my message!");
    msg.append("|");

    /*
    Field Name: ScenarioNo
    Data type: ASCII Numeric
    Justification: Any
    Length: 1
    Begin position: Variable
    End position: Variable
    Description: A one digit number to indicate the scenario of this test.

    Include 1 for this lab portion.
    Include 2 for second portion.
    Include 3 for last portion.
    */

    msg.append("3");
    msg.append("|"); // End of message

    /*
    Maximum length of request message 14610
    Maximum binary value in TCP header field = 14410, or HEX 0090
    */

    return msg;
}

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
    Insert a vertical �|� character at position 21
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
}

void responseFormat()
{
    /*
    Field Name: TCPHeader
    Data type Binary (Network, Big Endian, Byte Order)
    Justification: Right
    Length: 2
    Begin position: 0
    End position 1
    Description: Binary value that represents the length of the response message,
    excluding these two bytes.

    Field Name: MessageType
    Data type: ASCII Alphanumeric fixed �RSP�
    Justification: Any
    Length: 3
    Begin position: 2
    End position: 4
    Description: This will be a fixed three-character sequence of �RSP�, all in
    upper case.

    Insert a vertical �|� character at position 5

    Field Name: msTimeStamp
    Data type: ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 10
    Begin position: 6
    End position: Variable
    Description: A millisecond time stamp value from a running elapsed system
    timer on the server

    Insert a vertical �|� character in next position as a field separator

    Field Name: RequestID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Data from the RequestID field of the client�s request message

    Insert a vertical �|� character in next position as a field separator

    Field Name: StudentName
    Data type: ASCII Text
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: Data from the StudentName field of the client�s request message

    Insert a vertical �|� character in next position as a field separator

    Field Name: Student ID
    Data type: ASCII Aphanumeric
    Justification: Any
    Length: Variable with a maximum length of 7
    Begin position: Variable
    End position: Variable
    Description: Data from the StudentID field of the client�s request message

    Insert a vertical �|� character in next position as a field separator

    Field Name: ResponseDelay
    Data type: ASCII Numeric
    Justification: Right, space or zero filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: Data from the ResponseDelay field of the client�s request
    message

    Insert a vertical �|� character in next position as a field separator

    Field Name: ForeignHostIPAddress
    Data type: ASCII Alphanumeric
    Justification: Left, space filled to the right
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the server�s foreign host in dotted decimal
    notation

    Insert a vertical �|� character in next position as a field separator

    Field Name: ForeignHostServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server�s foreign host�s service port number, should always be
    2605.

    Insert a vertical �|� character in next position as a field separator

    Field Name: ServerSocketNumber
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server�s TCP socket number.

    Insert a vertical �|� character in next position as a field separator

    Field Name: ServerIPAddress
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 15
    Begin position: Variable
    End position: Variable
    Description: The IPv4 address of the server in dotted decimal notation.

    Insert a vertical �|� character in next position as a field separator

    Field Name: ServerServicePort
    Data type: ASCII Numeric
    Justification: Right, zero or space filled to the left
    Length: Variable with a maximum length of 5
    Begin position: Variable
    End position: Variable
    Description: The server�s service port number, should be 2605.

    Insert a vertical �|� character in next position as a field separator

    Field Name: ResponseID
    Data type: ASCII Alphanumeric
    Justification: Any
    Length: Variable with a maximum length of 20
    Begin position: Variable
    End position: Variable
    Description: A unique value generated by the server to exclusively identify
    responses.

    Insert a vertical �|� character in next position as a field separator

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

    Insert a vertical �|� character in next position as a field separator

    Maximum response length = 14610
    Maximum binary value in TCP header field = 14410, or HEX 0090
    */
}

// Store request messages to match them up with responses later
char req[10000][MAX_BUFSIZE];

//
// Transmits request numReq times.
//
void xmtThread(SOCKET sock, int numReq)
{
    for (int count = 0; count < numReq; ++count) {
        strcpy(req[count], const_cast<char*>(getMessage(sock).c_str())); // Copy message for later
        putReq(sock, req[count]);
        //Sleep(50);
    }
}

//
// Receives response numRsp times.
//
void rcvThread(SOCKET sock, int numRsp)
{
    setBlocking(sock, true); // Threaded, so block

    int count = 0;
    while (count < numRsp) {
        char temp[MAX_BUFSIZE] = "";
        getRsp(sock, temp);
        if (strlen(temp) > 0)
        {
            char * pch;
            pch = strtok(temp, "|");
            char messageType[4] = "";               // MessageType
            strcpy(messageType, pch);
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
            char foreignHostIpAddress[16] = "";     // ForeignHostIPAddress
            strcpy(foreignHostIpAddress, pch);
            pch = strtok(NULL, "|");
            int foreignHostServicePort = atoi(pch); // ForeignHostServicePort
            pch = strtok(NULL, "|");
            int serverSocketNumber = atoi(pch);     // ServerSocketNumber
            pch = strtok(NULL, "|");
            int serverIpAddress = atoi(pch);        // ServerIPAddress
            pch = strtok(NULL, "|");
            int serverServicePort = atoi(pch);      // ServerServicePort
            pch = strtok(NULL, "|");
            char responseId[21] = "";               // ResponseID
            strcpy(responseId, pch);
            pch = strtok(NULL, "|");
            int responseType = atoi(pch);           // ResponseType

            cout << "DEBUG: Received ID: " << requestId << " and status: " << responseType << endl;

            ++count;
        }
    }

    setBlocking(sock, false);
}

int main(int argc, char* argv[])
{
    bool hasIp = false;
    bool hasNumRequests = false;

    // If client, grab server IP and number of requests

    // If server, just indicate it is listening for connections

    if (argc > 1)
    {
        for (int arg = 0; arg < argc; ++arg)
        {
            // Check if argument is a server IP address
            struct sockaddr_in sock_addr;
            if (inet_pton(AF_INET, argv[arg], &sock_addr) > 0)
            {
                strcpy_s(server_IP, argv[arg]);
                hasIp = true;
            }
        }
        if (hasIp && !hasNumRequests || !hasIp && hasNumRequests)
        {
            cout << "ERROR: Numbers of requests and server IP must be given together." << endl;
            return 1;
        }
        else
        {
            cout << "INFO: Program started as server." << endl;
        }
    }
    else
    {
        cout << "INFO: Program started as client." << endl;
    }

    if (!initWinsock()) {
        cout << "DEBUG: Winsock initialization failed: " << WSAGetLastError() << endl;
        return 0;
    }

    //freopen("out.txt", "w", stdout); // Output to file

    SOCKET sock = connectTo(server_IP, DEFAULT_PORT, 3);

    if (sock == INVALID_SOCKET) {
        cout << "DEBUG: Unable to connect to server, shutting down..." << endl;
        return 0;
    }

    int numMsgs = 1;

    thread xmtThread(xmtThread, sock, numMsgs);
    thread rcvThread(rcvThread, sock, numMsgs);

    xmtThread.join();
    rcvThread.join();

    // Try receiving for another 20 seconds to check for latent and/or spurious responses

    int* shutdownStatus = shutdownSocket(sock);
    addTrailRecord(shutdownStatus[0], shutdownStatus[1], shutdownStatus[2]);
    delete[] shutdownStatus;

    return 0;
}