#include "Message.h"
#include <iostream>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdint.h>
#include <atlstr.h>
#include <thread>
using namespace std;

Message::Message(char * msg)
{
	/*
		char * messageType[4];
	char * studentName[21];
	char * studentId[8];
	char * foreignHostIpAddress[16];
	char * serverIpAddress[16];
	char * responseId[21];
	*/

	char * pch;
	pch = strtok(msg, "|");
	messageType = new char[4];
	strcpy(messageType, pch);
	pch = strtok(NULL, "|");
	msTimestamp = atol(pch);
	pch = strtok(NULL, "|");
	requestId = atoi(pch);
	pch = strtok(NULL, "|");
	studentName = new char[21];
	strcpy(studentName, pch);
	pch = strtok(NULL, "|");
	studentId = new char[8];
	strcpy(studentId, pch);
	pch = strtok(NULL, "|");
	responseDelay = atoi(pch);
	pch = strtok(NULL, "|");
	foreignHostIpAddress = new char[16];
	strcpy(foreignHostIpAddress, pch);
	pch = strtok(NULL, "|");
	foreignHostServicePort = atoi(pch);
	pch = strtok(NULL, "|");
	serverSocketNumber = atoi(pch);
	pch = strtok(NULL, "|");
	serverIpAddress = new char[16];
	strcpy(serverIpAddress, pch);
	pch = strtok(NULL, "|");
	serverServicePort = atoi(pch);
	pch = strtok(NULL, "|");
	responseId = new char[21];
	strcpy(responseId, pch);
	pch = strtok(NULL, "|");
	responseType = atoi(pch);
}

Message::~Message()
{
	delete[] messageType;
	delete[] studentName;
	delete[] studentId;
	delete[] foreignHostIpAddress;
	delete[] serverIpAddress;
	delete[] responseId;
}

long Message::getMsTimestamp()
{
	return msTimestamp;
}

int Message::getRequestId()
{
	return requestId;
}

const char * Message::to_cstr()
{
	string s;
	s.append(messageType);
	s.append("|");
	s.append(std::to_string(msTimestamp));
	s.append("|");
	s.append(std::to_string(requestId));
	s.append("|");
	s.append(studentName);
	s.append(studentId);
	s.append("|");
	s.append(std::to_string(responseDelay));
	s.append("|");
	s.append(foreignHostIpAddress);
	s.append("|");
	s.append(std::to_string(foreignHostServicePort));
	s.append("|");
	s.append(std::to_string(serverSocketNumber));
	s.append("|");
	s.append(serverIpAddress);
	s.append("|");
	s.append(std::to_string(serverServicePort));
	s.append("|");
	s.append(responseId);
	s.append("|");
	s.append(std::to_string(responseType));
	s.append("|");

	return s.c_str();
}