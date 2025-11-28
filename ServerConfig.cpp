#include "ServerConfig.hpp"

ServerConfig::ServerConfig() {}

ServerConfig::~ServerConfig() {}

void ServerConfig::setDirective(const std::string &key, const std::string &value) {
    _directives[key] = value;
}

std::string ServerConfig::getDirective(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = _directives.find(key);
    if (it != _directives.end())
        return it->second;
    return "";
}

std::map<std::string, std::string> ServerConfig::getAllDirectives() const {
    return _directives;
}

void ServerConfig::addLocation(const Location &location) {
    _locations.push_back(location);
}

std::vector<Location> ServerConfig::getLocations() const {
    return _locations;
}

size_t ServerConfig::getLocationCount() const {
    return _locations.size();
}
