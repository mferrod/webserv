#include "../includes/ConfigParser.hpp"
#include <iostream>

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

void ConfigParser::trimWhitespace(std::string &line) {
    size_t start = line.find_first_not_of(" \t\r\n");
    size_t end = line.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos)
        line = "";
    else
        line = line.substr(start, end - start + 1);
}

std::string ConfigParser::getToken(std::string &line) {
    trimWhitespace(line);
    size_t pos = line.find_first_of(" \t");
    
    if (pos == std::string::npos) {
        std::string token = line;
        line = "";
        return token;
    }
    
    std::string token = line.substr(0, pos);
    line = line.substr(pos);
    trimWhitespace(line);
    return token;
}

void ConfigParser::parseServer(std::ifstream &file) {
    ServerConfig server;
    std::string line;
    
    while (std::getline(file, line)) {
        trimWhitespace(line);
        
        if (line.empty() || line[0] == '#')
            continue;
        
        if (line == "}") {
            servers.push_back(server);
            return;
        }
        
        if (line.substr(0, 8) == "location") {
            std::string locationLine = line;
            locationLine.erase(0, 8);
            trimWhitespace(locationLine);
            size_t openBrace = locationLine.find('{');
            if (openBrace != std::string::npos) {
                std::string path = locationLine.substr(0, openBrace);
                trimWhitespace(path);
                parseLocation(file, server, path);
            }
            continue;
        }
        size_t semicolon = line.find(';');
        if (semicolon == std::string::npos) {
            std::cerr << "Error: missing semicolon in: " << line << std::endl;
            continue;
        }
        
        std::string directive = line.substr(0, semicolon);
        std::istringstream iss(directive);
        std::string key, value;
        
        iss >> key;
        std::getline(iss, value);
        trimWhitespace(value);
        
        if (key == "listen") {
            size_t colonPos = value.find(':');
            if (colonPos != std::string::npos) {
                std::string host = value.substr(0, colonPos);
                std::string port = value.substr(colonPos + 1);
                trimWhitespace(host);
                trimWhitespace(port);
                server.setDirective("listen", port);
                server.setDirective("host", host);
                continue;
            }
        }
        server.setDirective(key, value);
    }
}

void ConfigParser::parseLocation(std::ifstream &file, ServerConfig &server, const std::string &path) {
    Location location;
    location.setPath(path);
    std::string line;
    
    while (std::getline(file, line)) {
        trimWhitespace(line);
        
        if (line.empty() || line[0] == '#')
            continue;
        
        if (line == "}") {
            server.addLocation(location);
            return;
        }
        
        size_t semicolon = line.find(';');
        if (semicolon == std::string::npos)
            continue;
        
        std::string directive = line.substr(0, semicolon);
        std::istringstream iss(directive);
        std::string key, value;
        
        iss >> key;
        std::getline(iss, value);
        trimWhitespace(value);
        
        location.setDirective(key, value);
    }
}

bool ConfigParser::parseConfig(const std::string &filename) {
    std::string file_path = filename;
    std::ifstream file(file_path.c_str());
    
    if (!file.is_open()) {
        std::cerr << "Error: could not open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        trimWhitespace(line);
        
        if (line.empty() || line[0] == '#')
            continue;
        
        if (line == "server {") {
            parseServer(file);
        }
    }
    
    file.close();
    return true;
}

std::vector<ServerConfig> &ConfigParser::getServers() {
    return servers;
}