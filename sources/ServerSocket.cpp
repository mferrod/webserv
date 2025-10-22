#include "ServerSocket.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>

ServerSocket::ServerSocket(int port, const std::string& host) : _fd(-1), _port(port), _host(host) {
	// Crear el socket
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0) {
		std::cerr << "Error al crear el socket: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	// SO_REUSEADDR para evitar "Address already in use"
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		std::cerr << "Error al configurar el socket: " << strerror(errno) << std::endl;
		close(_fd);
		exit(EXIT_FAILURE);
	}

	// Configurar la dirección del socket
	memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
	_address.sin_port = htons(_port);
}

bool ServerSocket::bind() {
	return ::bind(_fd, (struct sockaddr*)&_address, sizeof(_address)) == 0;
}

bool ServerSocket::listen() {
	return ::listen(_fd, SOMAXCONN) == 0;
}

int ServerSocket::accept() {
	return ::accept(_fd, NULL, NULL);
}

ServerSocket::~ServerSocket() {
	// No cerrar el fd automáticamente para evitar problemas con copias
	// if (_fd >= 0) {
	//     close(_fd);
	// }
}

int ServerSocket::getFd() const {
	return _fd;
}

int ServerSocket::getPort() const {
	return _port;
}