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
    std::string _cgi_executor;  // /usr/bin/python3, /bin/bash, etc
    std::map<std::string, std::string> _env_vars;
    std::string _output;
    int _status_code;
    
    // Pipes para comunicación
    int _pipe_in[2];   // Para escribir al CGI (stdin)
    int _pipe_out[2];  // Para leer del CGI (stdout)
    
public:
    CGI(const HttpRequest &request, const Location &location);
    ~CGI();
    
    // Métodos principales
    bool execute();
    std::string getOutput() const { return _output; }
    int getStatusCode() const { return _status_code; }
    
private:
    // Preparación
    bool findScriptPath();
    bool findCGIExecutor();
    void setupEnvironment();
    
    // Ejecución
    bool createPipes();
    bool executeCGI();
    void closePipes();
    
    // Lectura/Escritura
    bool writeRequestBody();
    bool readCGIOutput();
    
    // Parsing de salida
    void parseCGIOutput();
};