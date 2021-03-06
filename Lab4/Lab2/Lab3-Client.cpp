#pragma once
#pragma comment (lib, "Ws2_32.lib")

#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <iomanip>
#include <thread>
#include <vector>
#include "ConnectionUtil.h"

using namespace std;

#define MAX_BUFSIZE 144

// Global variables
char server_IP[16];
int portNum = 0;
int numTx = 0;
int msDelay = 0;
u_int reqID = 0;
LARGE_INTEGER frequency;

char serverName[21] = "";

u_long firstReqTime = 0;
u_long firstRspTime = 0;
u_long finalReqTime = 0;
u_long finalRspTime = 0;

// Stores request messages
vector<char*> reqMsgs;

// Forward declarations
int genRequestID();
void xmtThread(SOCKET sock);
void rcvThread(SOCKET sock);
void addTrailRecord(int shutTransmit, int shutReceive, int shutSocket);
string getMessage(SOCKET sock);

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
            if (strcmp(argv[arg], "-ip") == 0)
            {
                struct sockaddr_in sock_addr;
                if (inet_pton(AF_INET, argv[++arg], &sock_addr) > 0)
                {
					//cout << "IP address: " << argv[arg] << endl;
					strcpy_s(server_IP, argv[arg]);
                }
                else
                {
                    //cout << "IP address does not match IPv4 format." << endl;
                    validArgs = false;
                }
            }
            else if (strcmp(argv[arg], "-port") == 0)
            {
                portNum = atoi(argv[++arg]);
                //cout << "Number transactions: " << numTx << endl;
            }
			else if (strcmp(argv[arg], "-tx") == 0)
            {
				numTx = atoi(argv[++arg]);
                //cout << "Number transactions: " << numTx << endl;
            }
			else if (strcmp(argv[arg], "-msDelay") == 0)
            {
				msDelay = atoi(argv[++arg]);
                //cout << "Request Delay: " << msDelay << endl;
				break;
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
        cout << "ERROR: arguments required: -ip <server ip> -port <port number> -tx <num transactions> -msDelay <delay>" << endl;
        return 1;
    }

    if (!initWinsock()) {
        cout << "ERROR: Winsock initialization failed: " << WSAGetLastError() << endl;
        return 1;
    }

    freopen("LabX.ScenarioX.WrightL.txt", "w", stdout); // Output to file

    SOCKET sock = connectTo(server_IP, portNum, 3);
    if (sock == INVALID_SOCKET) {
        cout << "ERROR: Unable to connect to server, shutting down..." << endl;
        return 1;
    }

	// Populate frequency information
	QueryPerformanceFrequency(&frequency);

	thread xmtThread(xmtThread, sock);
	thread rcvThread(rcvThread, sock);

	xmtThread.join();
	rcvThread.join();

    int* shutdownStatus = shutdownSocket(sock);
    addTrailRecord(shutdownStatus[0], shutdownStatus[1], shutdownStatus[2]);
    delete[] shutdownStatus;

	return 0;
}

//
// Transmits requests.
//
void xmtThread(SOCKET sock)
{	
	int count = 0;
	while (count < numTx)
	{
		// Delay between responses
		if (count != 0)
		{
			LARGE_INTEGER t1;
			QueryPerformanceCounter(&t1);
			while (1)
			{
				LARGE_INTEGER t2;
				QueryPerformanceCounter(&t2);
				double elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
				if (elapsedTime >= msDelay) break;
			}
		}

		char * msg = new char[MAX_BUFSIZE];
		strcpy(msg, const_cast<char*>(getMessage(sock).c_str()));
		reqMsgs.push_back(msg);
		
        while (1)
        {
            if (putReq(sock, msg)) break;
        }

        if (count == 0) firstReqTime = GetTickCount();
        else if (count == (numTx - 1)) finalReqTime = GetTickCount();

        ++count;
	}
}

//
// Receives responses.
//
void rcvThread(SOCKET sock)
{
    int count = 0;

    while (count < numTx)
	{
        char rsp[MAX_BUFSIZE] = "";
		if (getRsp(sock, rsp))
        {
            if (count == 0) firstRspTime = GetTickCount();
            else if (count == (numTx - 1)) finalRspTime = GetTickCount();

			char temp[MAX_BUFSIZE + 1];
			strcpy_s(temp, rsp);

            char * pch;
			pch = strtok(temp, "|");
            pch = strtok(NULL, "|");
            pch = strtok(NULL, "|");
            int requestId = stoi(pch);

            if (count == 0)
            {
                pch = strtok(NULL, "|");
                strcpy(serverName, pch);
            }

            // Process transaction according to request ID and output to log
            cout << reqMsgs.at(requestId) << endl;
			cout << rsp << endl;

            // Clear memory
            delete[] reqMsgs.at(requestId);

            ++count;
        }
    }
}

string getMessage(SOCKET sock)
{
    string msg = "";

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

    msg.append(std::to_string(reqID++));
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

    msg.append(std::to_string(portNum));
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

    msg.append("2");
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