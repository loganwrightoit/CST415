#pragma once
#include <string>

class Message
{
public:

	Message(char * msg);
	~Message();

	long getMsTimestamp();
	int getRequestId();

	void setResponseId(char * buffer);
	void setResponseType(int type);

	std::string to_string();

private:

	char * messageType;
	long msTimestamp;
	int requestId;
	char * studentName;
	char * studentId;
	int responseDelay;
	char * foreignHostIpAddress;
	int foreignHostServicePort;
	int serverSocketNumber;
	char * serverIpAddress;
	int serverServicePort;
	char * responseId;
	int responseType;

};

