#include "../includes/ServerSocket.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
#include <arpa/inet.h>

ServerSocket::ServerSocket(const ServerConfig& serverConfig) : _fd(-1), _host(serverConfig.getDirective("host")), _server_config(serverConfig) {
	std::stringstream ss(serverConfig.getDirective("listen"));
   	ss >> _port;
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0) {
		std::cerr << "Error al crear el socket: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "Error al configurar SO_REUSEADDR: " << strerror(errno) << std::endl;
		close(_fd);
		exit(EXIT_FAILURE);
	}

	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	if (!_host.empty()) {
		inet_pton(AF_INET, _host.c_str(), &_address.sin_addr);
	} else {
		_address.sin_addr.s_addr = INADDR_ANY;
	}
	_address.sin_port = htons(_port);
}

bool ServerSocket::bind() {
	int result = ::bind(_fd, (struct sockaddr*)&_address, sizeof(_address));
	if (result < 0) {
		std::cerr << "Error en bind() puerto " << _port << ": " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

bool ServerSocket::listen() {
	int result = ::listen(_fd, SOMAXCONN);
	if (result < 0) {
		std::cerr << "Error en listen() puerto " << _port << ": " << strerror(errno) << std::endl;
		return false;
	}
	return true;
}

int ServerSocket::accept() {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	
	int clientFd = ::accept(_fd, (struct sockaddr*)&client_addr, &client_len);
	if (clientFd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << "Error en accept() puerto " << _port << ": " << strerror(errno) << std::endl;
		}
		return -1;
	}
	
	return clientFd;
}

ServerSocket::~ServerSocket() {}

int ServerSocket::getFd() const {
	return _fd;
}

int ServerSocket::getPort() const {
	return _port;
}

std::string ServerSocket::getHost() const {
	return _host;
}

ServerConfig ServerSocket::getServerConfig() const {
	return _server_config;
}