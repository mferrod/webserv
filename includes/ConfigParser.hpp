#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Location.hpp"
#include "ServerConfig.hpp"

class ConfigParser {
private:
    std::vector<ServerConfig> servers;
    
    void trimWhitespace(std::string &line);
    std::string getToken(std::string &line);
    void parseServer(std::ifstream &file);
    void parseLocation(std::ifstream &file, ServerConfig &server, const std::string &path);

public:
    ConfigParser();
    ~ConfigParser();
    
    bool parseConfig(const std::string &filename);
    const std::vector<ServerConfig> &getServers() const;
};

#endif