#include "../includes/ServerManager.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

ServerManager::ServerManager(const std::string &configPath) {
    ConfigParser configParser;
    ConfigValidator configValidator;
    configParser.parseConfig(configPath);
    std::vector<ServerConfig> &serverConfigs = configParser.getServers();
    try {
        configValidator.validateServers(serverConfigs);
    } catch (const std::exception &e) {
        throw std::runtime_error(std::string("Error en configuración: ") + e.what());
    }
    for (size_t i = 0; i < serverConfigs.size(); i++) {
        servers.push_back(ServerSocket(serverConfigs[i]));
    }

    for (size_t i = 0; i < servers.size(); i++) {
        if (!servers[i].bind() || !servers[i].listen()) {
            std::cerr << "Error en bind en puerto " << servers[i].getServerConfig().getDirective("listen") << ": " << strerror(errno) << std::endl;
            std::stringstream ss;
            ss << "No se pudo iniciar servidor en puerto " << servers[i].getServerConfig().getDirective("listen");
            throw std::runtime_error(ss.str());
        }
        setNonBlocking(servers[i].getFd());
        fd_to_server_index[servers[i].getFd()] = i;
        
        pollfd pfd;
        pfd.fd = servers[i].getFd();
        pfd.events = POLLIN;
        pfd.revents = 0;
        poll_fds.push_back(pfd);
        std::cout << "Servidor iniciado en puerto " << servers[i].getServerConfig().getDirective("listen") << std::endl;
    }
    std::cout << "Todos los servidores iniciados y escuchando..." << std::endl;
}

void ServerManager::run() {
    while (true) {
        int ready = poll(&poll_fds[0], poll_fds.size(), 1000);

        if (ready == -1) {
            if (errno == EINTR) continue;
            std::cerr << "Error en poll(): " << strerror(errno) << std::endl;
            continue;
        }
        
        checkTimeouts();
        
        if (ready == 0) continue;

        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & (POLLIN | POLLOUT)) {
                if (fd_to_server_index.find(poll_fds[i].fd) != fd_to_server_index.end()) {
                    std::cout << "Nueva conexión entrante en FD: " << poll_fds[i].fd << std::endl;
                    acceptNewConnection(poll_fds[i].fd);
                } else {
                    int clientIndex = -1;
                    for (size_t j = 0; j < clients.size(); j++) {
                        if (clients[j].getFd() == poll_fds[i].fd) {
                            clientIndex = j;
                            break;
                        }
                    }
                    if (clientIndex != -1) {
                        handleClientEvent(clientIndex);
                    }
                }
            }
            
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (fd_to_server_index.find(poll_fds[i].fd) != fd_to_server_index.end()) {
                    std::cerr << "Error en socket servidor FD: " << poll_fds[i].fd << std::endl;
                } else {
                    for (size_t j = 0; j < clients.size(); j++) {
                        if (clients[j].getFd() == poll_fds[i].fd) {
                            removeClient(j);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void ServerManager::acceptNewConnection(int serverFd) {
    int serverIndex = findServerByFd(serverFd);
    if (serverIndex == -1) {
        std::cerr << "Error: servidor no encontrado para FD " << serverFd << std::endl;
        return;
    }
    
    int clientFd = servers[serverIndex].accept();
    if (clientFd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error en accept(): " << strerror(errno) << std::endl;
        }
        return;
    }

    setNonBlocking(clientFd);
    
    clients.push_back(ClientConnection(clientFd));
    
    pollfd clientPfd;
    clientPfd.fd = clientFd;
    clientPfd.events = POLLIN;
    clientPfd.revents = 0;
    poll_fds.push_back(clientPfd);
    
    client_timeouts[clientFd] = time(NULL);

    std::cout << "Nuevo cliente conectado desde servidor puerto " 
              << servers[serverIndex].getPort() << ". FD: " << clientFd << std::endl;
}

void ServerManager::handleClientEvent(int clientIndex) {
    if (clientIndex < 0 || (size_t)clientIndex >= clients.size()) return;
    
    ClientConnection &client = clients[clientIndex];
    int clientFd = client.getFd();
    
    client_timeouts[clientFd] = time(NULL);
    
    for (size_t i = 0; i < poll_fds.size(); i++) {
        if (poll_fds[i].fd == clientFd) {
            if (poll_fds[i].revents & POLLIN) {
                if (!client.readData()) {
                    removeClient(clientIndex);
                    return;
                }
                
                if (client.isRequestComplete() && !client.isParsed()) {
					client.parseRequest();
                    poll_fds[i].events = POLLOUT;
                }
            }
			if (poll_fds[i].revents & POLLOUT) {
				client.makeResponse(servers);
				std::string response = client.getResponseBuffer();
			    if (client.sendResponse(response)) {
			        removeClient(clientIndex);
			    } else {
			        std::cerr << "No se pudo enviar respuesta completa a FD " << clientFd << std::endl;
			    }
			}
					break;
		}
	}
}


void ServerManager::removeClient(int clientIndex) {
    if (clientIndex < 0 || (size_t)clientIndex >= clients.size()) return;
    
    int fd = clients[clientIndex].getFd();
    client_timeouts.erase(fd);
    clients.erase(clients.begin() + clientIndex);

    for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {
        if (it->fd == fd) {
            poll_fds.erase(it);
            break;
        }
    }
    
    close(fd);
    std::cout << "Cliente desconectado. FD: " << fd << std::endl;
}

void ServerManager::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> toRemove;
    
    for (std::map<int, time_t>::iterator it = client_timeouts.begin(); it != client_timeouts.end(); ++it) {
        if (now - it->second > CLIENT_TIMEOUT) {
            toRemove.push_back(it->first);
        }
    }
    
    for (size_t i = 0; i < toRemove.size(); i++) {
        for (size_t j = 0; j < clients.size(); j++) {
            if (clients[j].getFd() == toRemove[i]) {
                std::cout << "Cliente timeout. FD: " << toRemove[i] << std::endl;
                removeClient(j);
                break;
            }
        }
    }
}

int ServerManager::findServerByFd(int fd) {
    std::map<int, size_t>::iterator it = fd_to_server_index.find(fd);
    if (it != fd_to_server_index.end()) {
        return it->second;
    }
    return -1;
}

void ServerManager::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error obteniendo flags del FD " << fd << ": " << strerror(errno) << std::endl;
        return;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error configurando non-blocking en FD " << fd << ": " << strerror(errno) << std::endl;
    }
}
