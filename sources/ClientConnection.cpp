#include "../includes/ClientConnection.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#define BUFFER_SIZE 8192

ClientConnection::ClientConnection(int fd) 
	: _fd(fd), _complete(false), _response_sent(false), _response_offset(0), _parsed(false) {}

ClientConnection::~ClientConnection() {}

bool ClientConnection::readData() {
	char buf[BUFFER_SIZE];
	ssize_t bytesRead = recv(_fd, buf, BUFFER_SIZE - 1, 0);
	
	if (bytesRead < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return true;
		}
		std::cerr << "Error en recv(): " << strerror(errno) << std::endl;
		return false;
	}
	
	if (bytesRead == 0) {
		std::cout << "Cliente cerró la conexión. FD: " << _fd << std::endl;
		return false;
	}
	buf[bytesRead] = '\0';
	_buffer.append(buf, bytesRead);

	if (_buffer.find("\r\n\r\n") != std::string::npos) {
		_complete = true;
		std::cout << "Petición HTTP completa recibida de FD " << _fd << std::endl;
	}
	
	return true;
}

bool ClientConnection::sendResponse(const std::string &response) {
	if (_response_buffer.empty()) {
		_response_buffer = response;
		_response_offset = 0;
	}
	
	return sendPartialResponse();
}

bool ClientConnection::sendPartialResponse() {
	if (_response_sent || _response_buffer.empty()) {
		return _response_sent;
	}
	
	size_t remaining = _response_buffer.size() - _response_offset;
	if (remaining == 0) {
		_response_sent = true;
		return true;
	}
	
	ssize_t sent = send(_fd, _response_buffer.c_str() + _response_offset, remaining, 0);
	
	if (sent < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return false;
		}
		std::cerr << "Error en send(): " << strerror(errno) << std::endl;
		return false;
	}
	
	if (sent == 0) {
		return false;
	}
	
	_response_offset += sent;
	
	if (_response_offset >= _response_buffer.size()) {
		_response_sent = true;
		std::cout << "Respuesta completa enviada a FD " << _fd << std::endl;
		return true;
	}
	return false;
}

bool ClientConnection::parseRequest() {
    if (_parsed || !_complete) return _parsed;
    
	_request = HttpRequest();
	_request.parseRequest(_buffer);
	_response = HttpResponse(_request);    
    _parsed = true;
    return true;
}

void ClientConnection::makeResponse(std::vector<ServerSocket> &servers) {
	_response.handleRequest(servers);
	_response_buffer = _response.buildResponse();
}
