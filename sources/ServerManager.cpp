#include "../includes/ServerManager.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>  // strerror
#include <sstream>  // stringstream para conversiones
#include <cerrno>   // errno
#include <unistd.h> // close
#include <fcntl.h>  // fcntl para non-blocking

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
    // Cargar configuración con ConfigParser
    for (size_t i = 0; i < serverConfigs.size(); i++) {
        servers.push_back(ServerSocket(serverConfigs[i]));
    }

    // Configurar polling para servidores
    for (size_t i = 0; i < servers.size(); i++) {
        if (!servers[i].bind() || !servers[i].listen()) {
            std::cerr << "Error en bind en puerto " << servers[i].getServerConfig().getDirective("listen") << ": " << strerror(errno) << std::endl;
            std::stringstream ss;
            ss << "No se pudo iniciar servidor en puerto " << servers[i].getServerConfig().getDirective("listen");
            throw std::runtime_error(ss.str());
        }
        
        // Hacer el socket non-blocking
        setNonBlocking(servers[i].getFd());
        
        // Mapear FD a índice de servidor
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
    //std::cout << "Entrando al bucle principal del servidor..." << std::endl;
    while (true) {
        // Timeout de 1 segundo para poll
        int ready = poll(&poll_fds[0], poll_fds.size(), 1000);

        if (ready == -1) {
            if (errno == EINTR) continue; // Señal interrumpida, continuar
            std::cerr << "Error en poll(): " << strerror(errno) << std::endl;
            continue;
        }
        
        // Verificar timeouts de clientes cada vez
        checkTimeouts();
        
        if (ready == 0) continue; // Timeout, no hay eventos

        // Procesar eventos
        for (size_t i = 0; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & (POLLIN | POLLOUT)) {
                // Verificar si es un servidor o cliente
                if (fd_to_server_index.find(poll_fds[i].fd) != fd_to_server_index.end()) {
                    // Es un servidor, nueva conexión
                    std::cout << "Nueva conexión entrante en FD: " << poll_fds[i].fd << std::endl;
                    acceptNewConnection(poll_fds[i].fd);
                } else {
                    // Es un cliente, encontrar su índice
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
            
            // Verificar errores en los sockets
            if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Si es un servidor, error crítico
                if (fd_to_server_index.find(poll_fds[i].fd) != fd_to_server_index.end()) {
                    std::cerr << "Error en socket servidor FD: " << poll_fds[i].fd << std::endl;
                } else {
                    // Cliente desconectado o error
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
    // Encontrar qué servidor recibió la conexión
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

    // Hacer el cliente non-blocking
    setNonBlocking(clientFd);
    
    // Crear nueva conexión cliente
    clients.push_back(ClientConnection(clientFd));
    
    // Añadir a poll_fds
    pollfd clientPfd;
    clientPfd.fd = clientFd;
    clientPfd.events = POLLIN;
    clientPfd.revents = 0;
    poll_fds.push_back(clientPfd);
    
    // Registrar timeout
    client_timeouts[clientFd] = time(NULL);

    std::cout << "Nuevo cliente conectado desde servidor puerto " 
              << servers[serverIndex].getPort() << ". FD: " << clientFd << std::endl;
}

void ServerManager::handleClientEvent(int clientIndex) {
    if (clientIndex < 0 || (size_t)clientIndex >= clients.size()) return;
    
    ClientConnection &client = clients[clientIndex];
    int clientFd = client.getFd();
    
    // Actualizar timeout
    client_timeouts[clientFd] = time(NULL);
    
    // Manejar eventos de lectura
    for (size_t i = 0; i < poll_fds.size(); i++) {
        if (poll_fds[i].fd == clientFd) {
            if (poll_fds[i].revents & POLLIN) {
                if (!client.readData()) {
                    removeClient(clientIndex);
                    return;
                }
                
                // Si la petición está completa, parsear y preparar respuesta
                if (client.isRequestComplete() && !client.isParsed()) {
					client.parseRequest(); 										// Add server configuration here?
                    /* if (!client.parseRequest()) {
                        // Request malformada → 400 Bad Request
                        std::string errorResponse = 
                            "HTTP/1.1 400 Bad Request\r\n"
                            "Content-Type: text/html\r\n"
                            "Content-Length: 38\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "<html><body>Bad Request</body></html>";
                        poll_fds[i].events = POLLOUT;
                        client.sendResponse(errorResponse);
                        return;
                    } */
                    
                    // Cambiar eventos a POLLOUT para enviar respuesta
                    poll_fds[i].events = POLLOUT;
                }
            }
			if (poll_fds[i].revents & POLLOUT) {
				client.makeResponse(servers);								// Add server configuration here // Working on it
				std::string response = client.getResponseBuffer();
			    // Generar respuesta según el método HTTP
			    /* std::string method = client.getRequest().getMethod();
			    std::string path = client.getRequest().getPath();
			    std::string body;
			    std::string response;
			
			    if (method == "GET") {
			        body = "<html><body><h1>GET: " + path + "</h1></body></html>";
			    } 
			    else if (method == "POST") {
			        body = "<html><body><h1>POST: " + path + "</h1></body></html>";
			    } 
			    else if (method == "DELETE") {
			        body = "<html><body><h1>DELETE: " + path + "</h1></body></html>";
			    } 
			    else {
			        // Método no soportado → 405 Method Not Allowed
			        body = "<html><body>Method Not Allowed</body></html>";
			        std::stringstream ss;
			        ss << body.length();
			        response = 
			            "HTTP/1.1 405 Method Not Allowed\r\n"
			            "Content-Type: text/html\r\n"
			            "Content-Length: " + ss.str() + "\r\n"
			            "Connection: close\r\n"
			            "\r\n" + body;
			
			        if (client.sendResponse(response)) {
			            removeClient(clientIndex);
			        }
			        break;
			    } */
			
			    // Para métodos válidos (GET, POST, DELETE)
			    /* std::stringstream ss;
			    ss << body.length();
			    response = 
			        "HTTP/1.1 200 OK\r\n"
			        "Content-Type: text/html\r\n"
			        "Content-Length: " + ss.str() + "\r\n"
			        "Connection: close\r\n"
			        "\r\n" + body;
*/
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
    
    // Eliminar timeout
    client_timeouts.erase(fd);
    
    // Eliminar cliente
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

void ServerManager::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> toRemove;
    
    // Encontrar clientes que han expirado
    for (std::map<int, time_t>::iterator it = client_timeouts.begin(); it != client_timeouts.end(); ++it) {
        if (now - it->second > CLIENT_TIMEOUT) {
            toRemove.push_back(it->first);
        }
    }
    
    // Eliminar clientes expirados
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
