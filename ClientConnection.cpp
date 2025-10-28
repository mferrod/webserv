#include "ClientConnection.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

#define BUFFER_SIZE 8192

ClientConnection::ClientConnection(int fd) 
	: _fd(fd), _complete(false), _response_sent(false), _response_offset(0), _parsed(false) {}

ClientConnection::~ClientConnection() {
	// No cerrar automáticamente el FD - lo maneja ServerManager
}

bool ClientConnection::readData() {
	char buf[BUFFER_SIZE];
	ssize_t bytesRead = recv(_fd, buf, BUFFER_SIZE - 1, 0); // -1 para null terminator
	
	if (bytesRead < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No hay datos disponibles ahora, intentar después
			return true;
		}
		// Error real
		std::cerr << "Error en recv(): " << strerror(errno) << std::endl;
		return false;
	}
	
	if (bytesRead == 0) {
		// Cliente cerró la conexión
		std::cout << "Cliente cerró la conexión. FD: " << _fd << std::endl;
		return false;
	}

	// Añadir datos al buffer
	buf[bytesRead] = '\0';
	_buffer.append(buf, bytesRead);

	// Verificar si la petición HTTP está completa
	if (_buffer.find("\r\n\r\n") != std::string::npos) {
		_complete = true;
		std::cout << "Petición HTTP completa recibida de FD " << _fd << std::endl;
		// Debug: mostrar primeras líneas de la petición
		size_t firstLine = _buffer.find("\r\n");
		if (firstLine != std::string::npos) {
			std::cout << "Primera línea: " << _buffer.substr(0, firstLine) << std::endl;
		}
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
			// Socket no listo para escribir, intentar después
			return false;
		}
		// Error real
		std::cerr << "Error en send(): " << strerror(errno) << std::endl;
		return false;
	}
	
	if (sent == 0) {
		// No se pudo enviar nada
		return false;
	}
	
	_response_offset += sent;
	
	// Verificar si se envió toda la respuesta
	if (_response_offset >= _response_buffer.size()) {
		_response_sent = true;
		std::cout << "Respuesta completa enviada a FD " << _fd << std::endl;
		return true;
	}
	
	// Respuesta parcial enviada
	std::cout << "Respuesta parcial enviada a FD " << _fd 
			  << " (" << _response_offset << "/" << _response_buffer.size() << ")" << std::endl;
	return false; // Aún hay datos por enviar
}

bool ClientConnection::parseRequest() {
    if (_parsed || !_complete) return _parsed;
    
    // Separar headers y body
    size_t headerEnd = _buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        std::cerr << "Request malformada, sin separador de headers" << std::endl;
        return false;
    }
    
    std::string headerSection = _buffer.substr(0, headerEnd);
    _body = _buffer.substr(headerEnd + 4);
    
    // Parsear línea inicial (método, path, versión)
    size_t firstLineEnd = headerSection.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        std::cerr << "Primera línea HTTP malformada" << std::endl;
        return false;
    }
    
    std::string firstLine = headerSection.substr(0, firstLineEnd);
    std::stringstream ss(firstLine);
    
    ss >> _method >> _path >> _version;
    
    // Validar que tenemos las 3 partes
    if (_method.empty() || _path.empty() || _version.empty()) {
        std::cerr << "Línea HTTP incompleta: " << firstLine << std::endl;
        return false;
    }
    
    std::cout << "Parseado: " << _method << " " << _path << " " << _version << std::endl;
    
    // Parsear headers línea por línea
    size_t pos = firstLineEnd + 2;
    while (pos < headerSection.size()) {
        size_t lineEnd = headerSection.find("\r\n", pos);
        if (lineEnd == std::string::npos) break;
        
        std::string line = headerSection.substr(pos, lineEnd - pos);
        size_t colon = line.find(":");
        
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            
            // Limpiar espacios
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            _headers[key] = value;
        }
        
        pos = lineEnd + 2;
    }
    
    _parsed = true;
    return true;
}