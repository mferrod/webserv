#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <string>
#include <vector>
#include <map>
#include "ServerConfig.hpp"
#include "Location.hpp"

class ConfigValidator {
private:
    std::vector<std::string> _validMethods;
    std::vector<std::string> _validDirectives;
    std::vector<std::string> _validLocationDirectives;
    
    void initializeValidMethods();
    void initializeValidDirectives();
    void initializeValidLocationDirectives();
    
    bool isValidPort(int port) const;
    bool isValidHttpCode(int code) const;
    bool isValidMethod(const std::string &method) const;
    bool isValidDirective(const std::string &directive, bool isLocationContext) const;
    bool isValidPath(const std::string &path) const;
    bool isValidIP(const std::string &ip) const;
    int parsePort(const std::string &portStr) const;
    std::vector<int> parseErrorPages(const std::string &codes) const;
    
public:
    ConfigValidator();
    ~ConfigValidator();
    
    void validateServers(const std::vector<ServerConfig> &servers);
    void validateServerConfig(const ServerConfig &server);
    void validateLocation(const Location &location);
    void validateDirective(const std::string &key, const std::string &value, bool isLocationContext);
    void validateListen(const std::string &value);
    void validateHost(const std::string &value);
    void validateServerName(const std::string &value);
    void validateRoot(const std::string &value);
    void validateIndex(const std::string &value);
    void validateClientMaxBodySize(const std::string &value);
    void validateAllowedMethods(const std::string &value);
    void validateErrorPage(const std::string &value);
    void validateAutoindex(const std::string &value);
    void validateCgiPath(const std::string &value);
    void validateUploadDir(const std::string &value);
    void validateRedirect(const std::string &value);
    void validateReturn(const std::string &value);
};

#endif
