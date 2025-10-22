#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cerrno>

class ServerSocket {
private:
    int _fd;
    int _port;
    std::string _host;
    struct sockaddr_in _address;

public:
    ServerSocket(int port, const std::string& host = "127.0.0.1");
    ~ServerSocket();
    
    bool bind();
    bool listen();
    int accept();
    int getFd() const;
    int getPort() const;
};
