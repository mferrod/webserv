#pragma once
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
		std::string _method;
		std::string _path;
		std::string _version;
		std::map<std::string, std::string> _headers;
		std::string _body;
		bool _parsed;

		
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
		const std::string& getMethod() const { return _method; }
		const std::string& getPath() const { return _path; }
		const std::string& getVersion() const { return _version; }
		bool isParsed() const { return _parsed; }
		
	private:
		bool sendPartialResponse();
};