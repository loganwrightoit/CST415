#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <atlstr.h>
#include <thread>
#include "ConnectionModule.h"
#include "Message.h"

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_ADDR "192.168.101.210"
#define DEFAULT_PORT 2605
#define MAX_BUFSIZE 144

const int numMsgs = 100;
int requestID = 0;

// Store request messages to match them up with responses later
static char req[numMsgs][MAX_BUFSIZE];

// True if waiting for a response for RequestID
static bool waitForRsp[numMsgs];

int genRequestID()
{
	return requestID++;
}

string getReqMsg(SOCKET sock, int requestId, char * studentData, int scenarioNo)
{
	string msg = "";

	/*
	Field Name: MessageType
	Data type: ASCII Alphanumeric fixed “REQ”
	Justification: Any
	Length: 3
	Begin position: 2
	End position: 4
	Description: This is to be a fixed three-character sequence of “REQ”, all in
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

	msg.append(std::to_string(requestId));
	msg.append("|");
	
	/*
	Field Name: StudentName
	Data type: ASCII Text
	Justification: Left, space filled to the right
	Length: Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: Student’s last name followed by first letter of student’s first name.
	for example: “FindleyT” [without quotation marks, upper case
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
	Description: Student’s “918” ID number in the form of “dd-dddd”
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

	if (requestId == 25 || requestId == 50 || requestId == 75) {
		msg.append("4000");
	} else {
		msg.append("0");
	}

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
	Description: The client’s service port number that is being used in the TCP
	session with the instructor’s server.
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
	Description: The client’s TCP socket number that is being used in the TCP
	session with the instructor’s server.
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

	msg.append(DEFAULT_ADDR); // Need to add server IP address (Should be DEFAULT_ADDR)
	msg.append("|");

	/*
	Field Name: ForeignHostServicePort
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The foreign host’s (server’s) service port number that is being
	used in the TCP session with the student’s client (should always
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

	char stuData[19] = "";
	strcpy(stuData, studentData);
	msg.append(stuData);
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

	msg.append(std::to_string(scenarioNo));
	msg.append("|");

	/*
	Maximum length of request message 14610
	Maximum binary value in TCP header field = 14410, or HEX 0090
	*/

    return msg;
}

//
// Returns response message.
//
string getRspMsg(SOCKET sock, int inRequestId, char * inStudentName, char * inClientIp,
	int inClientPort, char * inStudentId, int inResponseDelay, char * inResponseId, int inResponseType)
{
	string msg = "";

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

	msg.append("RSP");
	msg.append("|");

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
	msg.append(std::to_string(timestamp));
	msg.append("|");

	/*
	Field Name: RequestID
	Data type: ASCII Alphanumeric
	Justification: Any
	Length Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: Data from the RequestID field of the client’s request message
	*/

	msg.append(std::to_string(inRequestId));
	msg.append("|");

	/*
	Field Name: StudentName
	Data type: ASCII Text
	Justification: Left, space filled to the right
	Length: Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: Data from the StudentName field of the client’s request message
	*/

	msg.append(inStudentName);
	msg.append("|");

	/*
	Field Name: Student ID
	Data type: ASCII Aphanumeric
	Justification: Any
	Length: Variable with a maximum length of 7
	Begin position: Variable
	End position: Variable
	Description: Data from the StudentID field of the client’s request message
	*/

	msg.append(inStudentId);
	msg.append("|");

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

	msg.append(std::to_string(inResponseDelay));
	msg.append("|");

	/*
	Field Name: ClientIPAddress
	Data type: ASCII Alphanumeric
	Justification: Left, space filled to the right
	Length: Variable with a maximum length of 15
	Begin position: Variable
	End position: Variable
	Description: The IPv4 address of the server’s foreign host in dotted decimal
	notation
	*/

	msg.append(inClientIp);
	msg.append("|");

	/*
	Field Name: ClientServicePort
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s foreign host’s service port number, should always be
	2605.
	*/

	msg.append(std::to_string(inClientPort));
	msg.append("|");

	/*
	Field Name: ServerSocketNumber
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s TCP socket number.
	*/

	msg.append(std::to_string(sock));
	msg.append("|");

	/*
	Field Name: ServerIPAddress
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 15
	Begin position: Variable
	End position: Variable
	Description: The IPv4 address of the server in dotted decimal notation.
	*/

	char serverIp[INET_ADDRSTRLEN] = "NULL";
	struct sockaddr_in sin;
	int addrlen = sizeof(sin);
	getsockname(sock, (struct sockaddr *)&sin, &addrlen);
	int local_port = ntohs(sin.sin_port);

	inet_ntop(AF_INET, &sin.sin_addr, serverIp, sizeof(serverIp));

	msg.append(serverIp);
	msg.append("|");

	/*
	Field Name: ServerServicePort
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s service port number, should be 2605.
	*/

	msg.append(std::to_string(local_port));
	msg.append("|");

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
	
	msg.append(inResponseId);
	msg.append("|");

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

	msg.append(std::to_string(inResponseType));
	msg.append("|");

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
}


void outputNormalRsp(Message reqMsg, Message rspMsg)
{
	cout << reqMsg.to_string() << endl;
	cout << rspMsg.to_string() << endl;
	cout << "---------------------------------------------------------------------" << endl;
}

void outputStandInRsp(Message reqMsg, string rspMsg)
{
	cout << reqMsg.to_string() << endl;
	cout << rspMsg << endl;
	cout << "---------------------------------------------------------------------" << endl;
}

void outputLatentRsp(Message reqMsg, Message rspMsg)
{
	cout << reqMsg.to_string() << endl;
	rspMsg.setResponseType(3);
	cout << rspMsg.to_string() << endl;
	cout << "---------------------------------------------------------------------" << endl;
}

void outputUnsolicitedRsp(Message reqMsg, Message rspMsg)
{
	cout << reqMsg.to_string() << endl;
	rspMsg.setResponseType(4);
	cout << rspMsg.to_string() << endl;
	cout << "---------------------------------------------------------------------" << endl;
}

//
// Waits a preset amount of time before issuing a stand-in response, or
// dropping the response from the expected responses.
//
void createClientRspThread(SOCKET sock, int id)
{
	Sleep(4000); // Sleep 4 seconds
	if (waitForRsp[id]) {
		char temp[MAX_BUFSIZE] = "";
		strcpy(temp, req[id]);
		Message reqMsg = Message(temp);
		string rspMsg = "RESPONSE TIMEOUT AFTER 4SEC FOR ID ";
		rspMsg.append(std::to_string(id));
		rspMsg.append(", THIS IS A STAND-IN RESPONSE");
		outputStandInRsp(reqMsg, rspMsg);
		return;
	}
	else {
		Sleep(20000); // Sleep 20 more seconds
		if (waitForRsp[id]) {
			// Don't wait for this response anymore.. it will log as unsolicited if received
			waitForRsp[id] = false;
		}
	}
}

//
// Transmits request numReq times.
//
void xmtThread(SOCKET sock)
{
	for (int count = 0; count < numMsgs; ++count) {
        strcpy(req[count], const_cast<char*>(getReqMsg(sock, genRequestID(), "Normal REQ", 3).c_str())); // Copy message for later
		putReq(sock, req[count]);
		thread createClientRspThread(createClientRspThread, sock, count); // Create timeout thread for rsp
		createClientRspThread.detach();
		Sleep(1000);
	}
}

//
// Receives response numRsp times.
//
void rcvThread(SOCKET sock)
{
	int count = 0;

	for (int count = 0; count < numMsgs; ++count)
	{
		char temp[MAX_BUFSIZE] = "";
		long ticks = GetTickCount();

		// Do nothing until response is received
		while (!getRsp(sock, temp))
		{
			ZeroMemory(temp, sizeof(temp));
			if (GetTickCount() - ticks > 24000) {
				return;
			}
		}

		Message rspMsg = Message(temp);

		int requestId = rspMsg.getRequestId();
		Message reqMsg = Message(req[requestId]);

		if (requestId >= numMsgs) {
			outputUnsolicitedRsp(reqMsg, rspMsg);
			continue;
		}

		if (!waitForRsp[requestId])
		{
			outputUnsolicitedRsp(reqMsg, rspMsg);
			continue;
		}
		else
		{
			waitForRsp[requestId] = false;

			// Awaiting a response, figure out if it's a normal or latent response
			long msElapsed = GetTickCount() - reqMsg.getMsTimestamp();
			if (msElapsed < 4000)
			{
				outputNormalRsp(reqMsg, rspMsg);
				//continue;
			}
			else if (msElapsed > 20000)
			{
				outputUnsolicitedRsp(reqMsg, rspMsg);
				//continue;
			}
			else
			{
				outputLatentRsp(reqMsg, rspMsg);
				//continue;
			}
		}
	}
}

int main()
{
    if (!initWinsock()) {
        cout << "DEBUG: Winsock initialization failed: " << WSAGetLastError() << endl;
        return 0;
    }

    freopen("out.txt", "w", stdout); // Output to file

    SOCKET sock = connectTo(DEFAULT_ADDR, DEFAULT_PORT, 3);
	setBlocking(sock, false);

    if (sock == INVALID_SOCKET) {
        cout << "DEBUG: Unable to connect to server, shutting down..." << endl;
        return 0;
    }

	for (int count = 0; count < numMsgs; ++count)
	{
		waitForRsp[count] = true;
	}

	thread rcvThread(rcvThread, sock);
	thread xmtThread(xmtThread, sock);
	xmtThread.join();
	rcvThread.join();

	Sleep(24000); // Let timeout threads finish up

    int* shutdownStatus = shutdownSocket(sock);
    addTrailRecord(shutdownStatus[0], shutdownStatus[1], shutdownStatus[2]);
    delete[] shutdownStatus;

	return 0;
}