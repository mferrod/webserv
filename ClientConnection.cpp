#include "ClientConnection.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

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

	if (_buffer.find("\r\n\r\n") != std::string::npos) {
		_complete = true;
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