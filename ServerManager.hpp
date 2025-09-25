// ServerManager.hpp
#pragma once
#include <vector>
#include <poll.h>
#include "ServerSocket.hpp"
#include "ClientConection.hpp"

class ServerManager {
private:
    std::vector<ServerSocket> servers;
    std::vector<ClientConnection> clients;
    std::vector<pollfd> poll_fds;

public:
    ServerManager(const std::string &configPath);
    void run(); // Bucle principal del servidor
private:
    void acceptNewConnection(int socketFd);
    void handleClientEvent(int clientIndex);
    void removeClient(int clientIndex);
};