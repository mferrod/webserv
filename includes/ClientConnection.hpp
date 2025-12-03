#pragma once
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerSocket.hpp"
#include <string>
#include <map>
#include <unistd.h>

class ClientConnection {
	private:
		int _fd;
		std::string _buffer;
		std::string _response_buffer;
		bool _complete;
		bool _response_sent;
		size_t _response_offset;
		bool _parsed;
		HttpRequest _request;
		HttpResponse _response;		
	public:
		ClientConnection(int fd);
		~ClientConnection();

		bool readData();
		bool sendResponse(const std::string &response);
		bool isRequestComplete() const { return _complete; }
		bool isResponseSent() const { return _response_sent; }
		int getFd() const { return _fd; }
		const std::string& getBuffer() const { return _buffer; }
		bool parseRequest();
		bool isParsed() const { return _parsed; }

		HttpRequest& getRequest() { return _request; }

		void makeResponse(std::vector<ServerSocket> &servers);
		const std::string& getResponseBuffer() const { return _response_buffer; }
		
	private:
		bool sendPartialResponse();
};