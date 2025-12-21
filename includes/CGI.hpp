#pragma once
#include <string>
#include <vector>
#include <map>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"

class CGI {
private:
    HttpRequest _request;
    Location _location;
    std::string _script_path;
    std::string _cgi_executor;
    std::map<std::string, std::string> _env_vars;
    std::string _output;
    int _status_code;
    
    int _pipe_in[2];
    int _pipe_out[2];
    
public:
    CGI(const HttpRequest &request, const Location &location);
    ~CGI();
    
    bool execute();
    std::string getOutput() const { return _output; }
    int getStatusCode() const { return _status_code; }
    
private:
    bool findScriptPath();
    bool findCGIExecutor();
    void setupEnvironment();
    
    bool createPipes();
    bool executeCGI();
    void closePipes();
    
    bool writeRequestBody();
    bool readCGIOutput();
    
    void parseCGIOutput();
};