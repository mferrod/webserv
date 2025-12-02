#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cerrno>
#include <vector>
#include "ServerConfig.hpp"

class ServerSocket {
private:
    int 					_fd;
    int 					_port;
    std::string 			_host;
    struct sockaddr_in		_address;
	ServerConfig			_server_config;

public:
    ServerSocket(int port, const std::string& host = "127.0.0.1");
    ~ServerSocket();
    
    bool bind();
    bool listen();
    int accept();
    int getFd() const;
    int getPort() const;
	std::string getHost() const;
	ServerConfig getServerConfig() const;
};
