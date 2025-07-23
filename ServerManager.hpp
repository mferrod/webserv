// ServerManager.hpp
#pragma once
#include <vector>
#include "ServerSocket.hpp"
#include "ConfigParser.hpp"
#include "ClientConnection.hpp"

class ServerManager {
private:
    std::vector<ServerSocket> servers;
    std::vector<ClientConnection> clients;
    std::vector<pollfd> poll_fds;

public:
    ServerManager(const std::string &configPath);
    void run(); // Bucle principal del servidor
private:
    void setupServers(const std::vector<ServerConfig> &configs);
    void acceptNewConnection(int socketFd);
    void handleClientEvent(int clientIndex);
};