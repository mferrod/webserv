// ServerManager.hpp
#pragma once
#include <vector>
#include <map>
#include <poll.h>
#include <ctime>
#include "ServerSocket.hpp"
#include "ClientConnection.hpp"
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"

class ServerManager {
private:
    std::vector<ServerSocket> servers;
    std::vector<ClientConnection> clients;
    std::vector<pollfd> poll_fds;
    std::map<int, time_t> client_timeouts;
    std::map<int, size_t> fd_to_server_index;
    
    static const int CLIENT_TIMEOUT = 60;

public:
    ServerManager(const std::string &configPath);
    void run();
private:
    void acceptNewConnection(int serverFd);
    void handleClientEvent(int clientIndex);
    void removeClient(int clientIndex);
    void checkTimeouts();
    int findServerByFd(int fd);
    void setNonBlocking(int fd);
};