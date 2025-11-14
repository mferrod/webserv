#include "../includes/ServerManager.hpp"
#include <iostream>


int main(void)
{
    HttpRequest request = HttpRequest();
    std::string rawRequest = "POST /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    request.parseRequest(rawRequest);  
    request.printRequest();

	HttpResponse response = HttpResponse(request);
	response.handleRequest();
	std::string rawResponse = response.buildResponse();
    std::cout << rawResponse << std::endl;

    return 0;
}

/* int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }

    try {
        ServerManager manager(argv[1]);  // Encapsula el parsing + arranque del server
        manager.run();                   // Entra al bucle de eventos con poll()
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} */