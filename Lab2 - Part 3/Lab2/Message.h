#pragma once
class Message
{
public:

	Message(char * msg);
	~Message();

	long getMsTimestamp();
	int getRequestId();

	const char * to_cstr();

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

