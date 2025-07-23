#include "ServerManager.hpp"

int main(int argc, char **argv) {
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
}