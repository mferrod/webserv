#include "ServerManager.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>  // strerror
#include <sstream>  // stringstream para conversiones
#include <cerrno>   // errno
#include <unistd.h> // close

ServerManager::ServerManager(const std::string &configPath) {
    (void)configPath; // Suprimir warning de parámetro no usado
    // Cargar configuración con ConfigParser
    // Aquí solo hacemos un ejemplo con servidor en puerto 8080
    servers.push_back(ServerSocket(8080));

    // Configurar polling para servidores
    for (size_t i = 0; i < servers.size(); i++) {
        if (!servers[i].bind() || !servers[i].listen()) {
            std::cerr << "Error en bind en puerto " << servers[i].getPort() << ": " << strerror(errno) << std::endl;
            std::stringstream ss;
            ss << "No se pudo iniciar servidor en puerto " << servers[i].getPort();
            throw std::runtime_error(ss.str());
        }
        pollfd pfd;
        pfd.fd = servers[i].getFd();
        pfd.events = POLLIN;
        poll_fds.push_back(pfd);
    }
    std::cout << "Servidor iniciado y escuchando..." << std::endl;
}

void ServerManager::run() {
    while (true) {
        int ready = poll(&poll_fds[0], poll_fds.size(), -1);

        if (ready == -1) {
            std::cerr << "Error en poll(): " << strerror(errno) << std::endl;
            continue;
        }

        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (i < servers.size()) {
                    acceptNewConnection(poll_fds[i].fd);
                    handleClientEvent(i - servers.size()); // For debugging
                } else {
                    handleClientEvent(i - servers.size());
                }
            }
        }
    }
}

void ServerManager::acceptNewConnection(int serverFd) {
    (void)serverFd; // Suprimir warning de parámetro no usado
    int clientFd = servers[0].accept();  // ejemplo, solo el primer servidor

    if (clientFd < 0) {
        std::cerr << "Error en accept(): " << strerror(errno) << std::endl;
        return;
    }

    clients.push_back(ClientConnection(clientFd));
    pollfd clientPfd;
    clientPfd.fd = clientFd;
    clientPfd.events = POLLIN;
    poll_fds.push_back(clientPfd);

    std::cout << "Nuevo cliente conectado. FD: " << clientFd << std::endl;
}

void ServerManager::handleClientEvent(int clientIndex) {
    if (clientIndex < 0 || (size_t)clientIndex >= clients.size()) return; 
    ClientConnection &client = clients[clientIndex];
    if (!client.readData()) {
        removeClient(clientIndex);
        return;
    }

    if (client.isRequestComplete()) {
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nHola";
        client.sendResponse(response);
        removeClient(clientIndex);
    }
}

void ServerManager::removeClient(int clientIndex) {
    int fd = clients[clientIndex].getFd();
    clients.erase(clients.begin() + clientIndex);

    // Borrar de poll_fds
    for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == fd) {
            poll_fds.erase(it);
            break;
        }
    }
    close(fd);
    std::cout << "Cliente desconectado. FD: " << fd << std::endl;
}
