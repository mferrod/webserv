#include "../includes/ServerManager.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc > 2) { 
        std::cerr << "Usage: ./webserv <config_file>" << std::endl;
        return 1;
    }

    try {
        std::string configFile = (argc == 2) ? argv[1] : "./configs/evaluation.conf";
		ServerManager manager(configFile);
        manager.run(); // Entra al bucle de eventos con poll()
    } catch (const std::exception &e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}