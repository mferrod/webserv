#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include "Location.hpp"

class ServerConfig {
private:
    std::map<std::string, std::string> _directives;
    std::vector<Location> _locations;

public:
    ServerConfig();
    ~ServerConfig();
    
    void setDirective(const std::string &key, const std::string &value);
    std::string getDirective(const std::string &key) const;
    std::map<std::string, std::string> getAllDirectives() const;
    
    void addLocation(const Location &location);
    std::vector<Location> getLocations() const;
    size_t getLocationCount() const;
};

#endif
