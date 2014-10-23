#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <atlstr.h>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_ADDR "192.168.101.210"
#define DEFAULT_PORT "2605"
#define DEFAULT_BUFLEN 256

WSADATA wsaData;
SOCKET _socket = INVALID_SOCKET;
int requestID = 0;
struct addrinfo *result = NULL, *ptr = NULL, hints;

//
// Shuts down connection and socket and returns array of results.
// First parameter is receive close status, second is send close status, third is socket close status.
//
int* _shutdown()
{
    int * results = new int[3];

	results[0] = shutdown(_socket, SD_RECEIVE); // Close receiving side
	if (results[0] == SOCKET_ERROR) {
		printf("ERROR: Socket receive shutdown failed: %d\n", WSAGetLastError());
	}
	results[1] = shutdown(_socket, SD_SEND); // Close sending side
    if (results[1] == SOCKET_ERROR) {
		printf("ERROR: Socket send shutdown failed: %d\n", WSAGetLastError());
	}

    results[2] = closesocket(_socket);
	WSACleanup();

    return results;
}

int genRequestID()
{
	return requestID++;
}

//
// Transmit message over connection.
//
void transmit(const CHAR * inStr)
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

    // Send message
    int _result = send(_socket, s.c_str(), s.length(), 0);
	if (_result == SOCKET_ERROR) {
        cout << "DEBUG: Error requesting: " << WSAGetLastError() << ", shutting down connection." << endl;
		_shutdown();
    } else {
        cout << inStr << endl;
    }
}

//
// Receive transaction data and store in buffer.
//
boolean receive()
{
	// Receive first two bytes (message length)
    u_short messageLength;
	int inBytes = recv(_socket, (char*)&messageLength, sizeof(messageLength), 0);

	if (inBytes == WSAEWOULDBLOCK) {
		return false;
	}

	if (inBytes > 0) {
		char buffer[DEFAULT_BUFLEN] = "";
		int _msg = recv(_socket, buffer, ntohs(messageLength), 0);
		if (_msg > 0) {
			cout << buffer << endl;
			return true;
		}
	} else if (inBytes == 0) {
		//cout << "DEBUG: Server gracefully closed connection.\n";
	} else {
		//cout << "DEBUG: Error receiving: " << WSAGetLastError() << ", shutting down connection." << endl;
		//_shutdown();
	}

	return false;
}

//
// Initializes WSA and makes a connection to nodeName (IP) provided.
//
void startup()
{
	// Initialize Winsock
	int _result = WSAStartup(MAKEWORD(2, 2), &wsaData); // Winsock v2.2
	if (_result != 0) {
		cout << "DEBUG: WSAStartup failed: " << _result << endl;
		return;
	}

	ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	_result = getaddrinfo(DEFAULT_ADDR, DEFAULT_PORT, &hints, &result);
	if (_result != 0) {
        cout << "DEBUG: Getaddrinfo failed: " << _result << endl;
		WSACleanup();
		return;
	}

	// Create a SOCKET for connecting to server
    _socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (_socket == INVALID_SOCKET) {
        cout << "DEBUG: Socket creation failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// Connect to server.
    _result = connect(_socket, result->ai_addr, (int)result->ai_addrlen);
	if (_result != 0) {
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (_socket == INVALID_SOCKET) {
        cout << "DEBUG: Unable to connect to server at " << DEFAULT_ADDR << endl;
		WSACleanup();
		return;
	}
}

string getMessage()
{
	string msg = "";
	int reqId = genRequestID();

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

	msg.append(std::to_string(reqId));
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

	if (reqId == 25 | reqId == 50 | reqId == 75) {
		msg.append("3000");
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
    getsockname(_socket, (struct sockaddr *)&sin, &addrlen);
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

    msg.append(std::to_string(_socket));
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

	msg.append(DEFAULT_PORT);
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
	Data type: ASCII Alphanumeric fixed “RSP”
	Justification: Any
	Length: 3
	Begin position: 2
	End position: 4
	Description: This will be a fixed three-character sequence of “RSP”, all in
	upper case.

	Insert a vertical ‘|’ character at position 5

	Field Name: msTimeStamp
	Data type: ASCII Numeric
	Justification: Right, space or zero filled to the left
	Length: Variable with a maximum length of 10
	Begin position: 6
	End position: Variable
	Description: A millisecond time stamp value from a running elapsed system
	timer on the server

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: RequestID
	Data type: ASCII Alphanumeric
	Justification: Any
	Length Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: Data from the RequestID field of the client’s request message

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: StudentName
	Data type: ASCII Text
	Justification: Left, space filled to the right
	Length: Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: Data from the StudentName field of the client’s request message

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: Student ID
	Data type: ASCII Aphanumeric
	Justification: Any
	Length: Variable with a maximum length of 7
	Begin position: Variable
	End position: Variable
	Description: Data from the StudentID field of the client’s request message

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ResponseDelay
	Data type: ASCII Numeric
	Justification: Right, space or zero filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: Data from the ResponseDelay field of the client’s request
	message

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ForeignHostIPAddress
	Data type: ASCII Alphanumeric
	Justification: Left, space filled to the right
	Length: Variable with a maximum length of 15
	Begin position: Variable
	End position: Variable
	Description: The IPv4 address of the server’s foreign host in dotted decimal
	notation

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ForeignHostServicePort
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s foreign host’s service port number, should always be
	2605.

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ServerSocketNumber
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s TCP socket number.

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ServerIPAddress
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 15
	Begin position: Variable
	End position: Variable
	Description: The IPv4 address of the server in dotted decimal notation.

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ServerServicePort
	Data type: ASCII Numeric
	Justification: Right, zero or space filled to the left
	Length: Variable with a maximum length of 5
	Begin position: Variable
	End position: Variable
	Description: The server’s service port number, should be 2605.

	Insert a vertical ‘|’ character in next position as a field separator

	Field Name: ResponseID
	Data type: ASCII Alphanumeric
	Justification: Any
	Length: Variable with a maximum length of 20
	Begin position: Variable
	End position: Variable
	Description: A unique value generated by the server to exclusively identify
	responses.

	Insert a vertical ‘|’ character in next position as a field separator

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

	Insert a vertical ‘|’ character in next position as a field separator

	Maximum response length = 14610
	Maximum binary value in TCP header field = 14410, or HEX 0090
	*/
}

/*
Log File Organization:

To receive credit for this lab assignment, students are to submit their log files to the
instructor. Please submit the logs as soon as you are comfortable with the results. Do not wait
to accumulate log files from all three scenarios before submitting for the first scenario. There is
to be only one log file for each scenario organized as follows:

	1.	Each line item in the log file represents either a request message or response message,
		except for the last line item which is a log trailer record.
	2.	Each line item is to be separated with a <CR><LF> character sequence.
	3.	Request line items are to be ordered in the sequence the request messages were
		issued. Hence, the request msTimeStamp field should continuously increment.
	4.	Response line items are to be ordered in the sequence they were received. Hence, the
		response msTimeStamp field should continuously increment.
	5.	The last line item is to be a log trailer record formatted in accordance with the Trailer
		Record Format and Content section of this assignment.

The last line item of each log file is to be formatted with the following items of data:

Field Name: Date
Data type: ASCII, in the form of MMDDYYYY
Justification: Any
Length: 8
Begin position: 0
End position: 7
Description: Date of completion of lab scenario

Insert a vertical ‘|’ character at position 8

Field Name: Time
Data type: ASCII, in the form of HHMMSS [24 hr. notation]
Justification: Any
Length: 6
Begin position: 9
End position: 14
Description: Time of completion of lab scenario

Insert a vertical ‘|’ character at position 15

Field Name: RcvShutdownStatus
Data type: ASCII Numeric
Justification: Right, space or zero filled to the left
Length: 5
Begin position: 16
End position: 20
Description: Return status from issuing a shutdown of the receive half session.

Insert a vertical ‘|’ character at position 21

Field Name: XmtShutdownStatus
Data type: ASCII Numeric
Justification: Right, space or zero filled to the left
Length: 5
Begin position: 22
End position: 26
Description: Return status from issuing a shutdown of the transmit half
session.

Insert a vertical ‘|’ character at position 27

Field Name: CloseStatus
Data type: ASCII Numeric
Justification: Right, space or zero filled to the left
Length: 5
Begin position: 28
End position: 32
Description: Return status from issuing a socket close command
*/

int recvCheck()
{
	boolean receive();

	if (receive()) {
		return 1;
	}
	else {
		return 0;
	}
}

int main()
{
    freopen("out.txt", "w", stdout); // Output to file
    //printf("DEBUG: Lab starting up...\n");
	startup();

	// Set socket to non-blocking mode
	u_long iMode = 1;
	ioctlsocket(_socket, FIONBIO, &iMode);

    //printf("DEBUG: Remote connection successful.\n");

	int numMsgs = 100;
	int recvCount = 0;

	// Begin sending and check for responses in-between

	while (requestID < numMsgs) {
		string msg = getMessage();
		transmit(msg.c_str());
		Sleep(50);
		recvCount += recvCheck(); // Check for incoming response
	}

	// Finish up receiving responses

	while (recvCount < numMsgs) {
		recvCount += recvCheck();
	}

    //printf("\nDEBUG: Lab complete, shutting down connection.\n");

    int* shutdownStatus = _shutdown();
    addTrailRecord(shutdownStatus[0], shutdownStatus[1], shutdownStatus[2]);
    delete[] shutdownStatus;

	return 0;
}