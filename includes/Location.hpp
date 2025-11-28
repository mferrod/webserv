#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <string>
#include <map>

class Location {
private:
    std::string _path;
    std::map<std::string, std::string> _directives;

public:
    Location();
    ~Location();
    
    void setPath(const std::string &path);
    std::string getPath() const;
    
    void setDirective(const std::string &key, const std::string &value);
    std::string getDirective(const std::string &key) const;
    std::map<std::string, std::string> getAllDirectives() const;
};

#endif
