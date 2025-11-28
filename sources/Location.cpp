#include "Location.hpp"

Location::Location() {}

Location::~Location() {}

void Location::setPath(const std::string &path) {
    _path = path;
}

std::string Location::getPath() const {
    return _path;
}

void Location::setDirective(const std::string &key, const std::string &value) {
    _directives[key] = value;
}

std::string Location::getDirective(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = _directives.find(key);
    if (it != _directives.end())
        return it->second;
    return "";
}

std::map<std::string, std::string> Location::getAllDirectives() const {
    return _directives;
}
