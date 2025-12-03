#include <iostream>
#include "ConfigParser.hpp"
#include "ConfigValidator.hpp"

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " config_file" << std::endl;
        return 1;
    }
    
    try {
        ConfigParser parser;
        std::string configFile = (argc == 2) ? argv[1] : "default_config.conf";
        parser.parseConfigFile(configFile);
        const std::vector<ServerConfig> &servers = parser.getServers();
        
        // Validar configuración
        ConfigValidator validator;
        validator.validateServers(servers);
        
        std::cout << "Configuration is valid!" << std::endl;
        std::cout << "Parsed " << servers.size() << " server(s)" << std::endl;
        
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}