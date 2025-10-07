#include "ClientConnection.hpp"

#define BUFFER_SIZE 8192

ClientConnection::ClientConnection(int fd) : _fd(fd), _complete(false) {}

ClientConnection::~ClientConnection() {
	if (_fd >= 0) close(_fd);
}

bool ClientConnection::readData() {
	char buf[BUFFER_SIZE];
	ssize_t bytesRead = recv(_fd, buf, BUFFER_SIZE, 0);
	if (bytesRead <= 0) return false;

	_buffer.append(buf, bytesRead);
	// For debugging
	/* _buffer = "GET /hola.html HTTP/1.1\r\n"
	"Host: localhost\r\n"
	"User-Agent: test-client\r\n"
	"\r\n"
	"<html><body>Hello, world!</body></html>"; */
	if (_buffer.find("\r\n\r\n") != std::string::npos) {
		_complete = true;
		_request = parseRequest(_buffer);
	}
	return true;
}

bool ClientConnection::sendResponse(const std::string &response) {
	ssize_t totalSent = 0;
	ssize_t toSend = response.size();

	while (totalSent < toSend) {
		ssize_t sent = send(_fd, response.c_str() + totalSent, toSend - totalSent, 0);
		if (sent <= 0) return false;
		totalSent += sent;
	}
	return true;
}

HttpRequest ClientConnection::parseRequest(const std::string &rawRequest) {
	HttpRequest request;
	size_t pos = 0;
	size_t lineEnd = rawRequest.find("\r\n");

	if (lineEnd == std::string::npos) return request;

	std::string requestLine = rawRequest.substr(0, lineEnd);
	pos = lineEnd + 2;

	size_t methodEnd = requestLine.find(' ');
	if (methodEnd == std::string::npos) return request;
	request.method = requestLine.substr(0, methodEnd);

	size_t pathEnd = requestLine.find(' ', methodEnd + 1);
	if (pathEnd == std::string::npos) return request;
	request.path = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);

	request.version = requestLine.substr(pathEnd + 1);

	while (true) {
		lineEnd = rawRequest.find("\r\n", pos);
		if (lineEnd == std::string::npos || lineEnd == pos) break;

		std::string headerLine = rawRequest.substr(pos, lineEnd - pos);
		size_t colonPos = headerLine.find(':');
		if (colonPos != std::string::npos) {
			std::string headerName = headerLine.substr(0, colonPos);
			std::string headerValue = headerLine.substr(colonPos + 1);
			while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t')) {
				headerValue.erase(0, 1);
			}
			request.headers[headerName] = headerValue;
		}
		pos = lineEnd + 2;
	}

	if (pos < rawRequest.size()) {
		request.body = rawRequest.substr(pos);
	}

	return request;
}