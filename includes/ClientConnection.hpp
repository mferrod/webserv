#pragma once
#include <string>
#include <unistd.h>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class ClientConnection
{
private:
	int			 _fd;
	std::string	 _buffer;
	bool		 _complete;
	HttpRequest	 _request;
	HttpResponse _response;

public:
	ClientConnection(int fd);
	~ClientConnection();

	bool readData();
	bool sendResponse(const std::string &response);
	bool isRequestComplete() const { return _complete; }
	int getFd() const { return _fd; }
};