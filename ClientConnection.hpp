#pragma once
#include <string>
#include <unistd.h>

class ClientConnection
{
private:
	int _fd;
	std::string _buffer;
	bool _complete;

public:
	ClientConnection(int fd);
	~ClientConnection();

	bool readData();
	bool sendResponse(const std::string &response);
	bool isRequestComplete() const { return _complete; }
	int getFd() const { return _fd; }
};