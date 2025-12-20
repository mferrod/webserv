#include "../includes/CGI.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>

CGI::CGI(const HttpRequest &request, const Location &location)
    : _request(request), _location(location), _status_code(200) {
    _pipe_in[0] = -1;
    _pipe_in[1] = -1;
    _pipe_out[0] = -1;
    _pipe_out[1] = -1;
}

CGI::~CGI() {
    closePipes();
}

bool CGI::findScriptPath() {
    std::string root = _location.getDirective("root");
    if (root.empty())
        root = ".";
    
    std::string request_path = _request.getPath();
    std::string location_path = _location.getPath();
    
    std::string file_part = request_path;
    
    if (location_path[0] != '~') {
        if (request_path.find(location_path) == 0) {
            file_part = request_path.substr(location_path.length());
        }
    } else {
        size_t last_slash = request_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            file_part = request_path.substr(last_slash + 1);
        } else {
            file_part = request_path;
        }
    }
    
    if (!file_part.empty() && file_part[0] == '/')
        file_part = file_part.substr(1);
    
    if (root == "." || root == "./")
        _script_path = root + (root[root.length()-1] == '/' ? "" : "/") + file_part;
    else if (root == "/")
        _script_path = "/" + file_part;
    else
        _script_path = root + "/" + file_part;
    
    std::cout << "CGI: Script path determined as: " << _script_path << std::endl;
    
    if (access(_script_path.c_str(), F_OK) != 0) {
        std::cerr << "CGI: Script no encontrado: " << _script_path << std::endl;
        _status_code = 404;
        return false;
    }
    
    return true;
}

bool CGI::findCGIExecutor() {
    size_t dot_pos = _request.getPath().find_last_of('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "CGI: No se puede determinar tipo de script" << std::endl;
        _status_code = 500;
        return false;
    }
    
    std::string extension = _request.getPath().substr(dot_pos);    
    std::string cgi_path = _location.getDirective("cgi_path");
    std::string cgi_ext = _location.getDirective("cgi_ext");
    
    if (cgi_path.empty()) {
        std::cerr << "CGI: cgi_path no configurado" << std::endl;
        _status_code = 500;
        return false;
    }
    
    if (extension == ".bla") {
        _cgi_executor = cgi_path;
    }
    else {
        _cgi_executor = cgi_path;
    }
    
    if (_cgi_executor.empty()) {
        std::cerr << "CGI: No se encontró ejecutor" << std::endl;
        _status_code = 500;
        return false;
    }
    
    if (access(_cgi_executor.c_str(), X_OK) != 0) {
        std::cerr << "CGI: Ejecutor no existente o no ejecutable: " << _cgi_executor << std::endl;
        _status_code = 500;
        return false;
    }
    
    return true;
}

void CGI::setupEnvironment() {
    _env_vars["REQUEST_METHOD"] = _request.getMethod();
    
    std::string path = _request.getPath();
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        _env_vars["QUERY_STRING"] = path.substr(query_pos + 1);
    } else {
        _env_vars["QUERY_STRING"] = "";
    }
    
    std::string script_name = _request.getPath();
    size_t query_pos_script = script_name.find('?');
    if (query_pos_script != std::string::npos) {
        script_name = script_name.substr(0, query_pos_script);
    }
    _env_vars["SCRIPT_NAME"] = script_name;
    
    std::string location_path = _location.getPath();
    
    if (location_path[0] == '~') {
        _env_vars["PATH_INFO"] = script_name;
    } else {
        std::string request_path_clean = script_name;
        
        if (request_path_clean.find(location_path) == 0) {
            request_path_clean = request_path_clean.substr(location_path.length());
        }
        
        size_t script_name_end = request_path_clean.find_first_of('/', 1);
        if (script_name_end != std::string::npos) {
            _env_vars["PATH_INFO"] = request_path_clean.substr(script_name_end);
        } else {
            _env_vars["PATH_INFO"] = "";
        }
    }
    
    _env_vars["REQUEST_URI"] = script_name;
    
    _env_vars["SERVER_PROTOCOL"] = "HTTP/1.1";
    
    _env_vars["SERVER_NAME"] = _request.getHost();
    std::ostringstream port_ss;
    port_ss << _request.getPort();
    _env_vars["SERVER_PORT"] = port_ss.str();
    
    if (_request.getMethod() == "POST") {
        std::ostringstream len_ss;
        len_ss << _request.getBody().size();
        _env_vars["CONTENT_LENGTH"] = len_ss.str();
        
        std::map<std::string, std::string> headers = _request.getHeaders();
        if (headers.find("content-type") != headers.end()) {
            _env_vars["CONTENT_TYPE"] = headers["content-type"];
        }
    } else {
        _env_vars["CONTENT_LENGTH"] = "0";
    }
    
    std::map<std::string, std::string> headers = _request.getHeaders();
    for (std::map<std::string, std::string>::iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string key = "HTTP_" + it->first;
        for (size_t i = 0; i < key.length(); i++) {
            if (key[i] == '-') key[i] = '_';
            key[i] = toupper(key[i]);
        }
        _env_vars[key] = it->second;
    }
    
    _env_vars["REMOTE_ADDR"] = _request.getHost(); // Placeholder??
    std::cout << "CGI: Environment variables configured:" << std::endl;
    for (std::map<std::string, std::string>::iterator it = _env_vars.begin();
         it != _env_vars.end(); ++it) {
        std::cout << "  " << it->first << "=" << it->second << std::endl;
    }
}

bool CGI::createPipes() {
    if (pipe(_pipe_in) < 0) {
        std::cerr << "CGI: Error creando pipe de entrada" << std::endl;
        return false;
    }
    
    if (pipe(_pipe_out) < 0) {
        std::cerr << "CGI: Error creando pipe de salida" << std::endl;
        close(_pipe_in[0]);
        close(_pipe_in[1]);
        return false;
    }
    
    return true;
}

void CGI::closePipes() {
    if (_pipe_in[0] != -1) close(_pipe_in[0]);
    if (_pipe_in[1] != -1) close(_pipe_in[1]);
    if (_pipe_out[0] != -1) close(_pipe_out[0]);
    if (_pipe_out[1] != -1) close(_pipe_out[1]);
    
    _pipe_in[0] = _pipe_in[1] = -1;
    _pipe_out[0] = _pipe_out[1] = -1;
}

bool CGI::executeCGI() {
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "CGI: Error en fork()" << std::endl;
        _status_code = 500;
        return false;
    }
    
    if (pid == 0) {
        
        dup2(_pipe_in[0], STDIN_FILENO);
        close(_pipe_in[0]);
        close(_pipe_in[1]);
        
        dup2(_pipe_out[1], STDOUT_FILENO);
        close(_pipe_out[0]);
        close(_pipe_out[1]);
        
        char *argv[2];
        argv[0] = const_cast<char*>("ubuntu_cgi_tester");
        argv[1] = NULL;
        
        std::vector<char*> envp;
        
        for (std::map<std::string, std::string>::iterator it = _env_vars.begin();
             it != _env_vars.end(); ++it) {
            std::string env_string = it->first + "=" + it->second;
            envp.push_back(strdup(env_string.c_str()));
        }
        envp.push_back(NULL);
        
        execve(argv[0], argv, &envp[0]);
        
        std::cerr << "CGI: Error en execve(): " << strerror(0) << std::endl;
        exit(1);
    }
    else {
        close(_pipe_in[0]);
        close(_pipe_out[1]);
        
        if (!writeRequestBody()) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            _status_code = 500;
            return false;
        }
        
        close(_pipe_in[1]);
        _pipe_in[1] = -1;
        
        if (!readCGIOutput()) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            _status_code = 500;
            return false;
        }
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::cerr << "CGI: El script terminó con error: " 
                      << WEXITSTATUS(status) << std::endl;
            _status_code = 500;
            return false;
        }
        
        parseCGIOutput();
        
        return true;
    }
}

bool CGI::writeRequestBody() {
    if (_request.getMethod() != "POST" || _request.getBody().empty()) {
        return true;
    }
    
    const std::string &body = _request.getBody();
    size_t total_written = 0;
    
    while (total_written < body.size()) {
        ssize_t written = write(_pipe_in[1], 
                                body.c_str() + total_written,
                                body.size() - total_written);
        
        if (written < 0) {
            std::cerr << "CGI: Error escribiendo body al pipe" << std::endl;
            return false;
        }
        
        total_written += written;
    }
    
    return true;
}

bool CGI::readCGIOutput() {
    char buffer[4096];
    ssize_t bytes_read;
    
    _output.clear();
    
    while ((bytes_read = read(_pipe_out[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        _output.append(buffer, bytes_read);
    }
    
    if (bytes_read < 0) {
        std::cerr << "CGI: Error leyendo salida del pipe" << std::endl;
        return false;
    }
    
    return true;
}

void CGI::parseCGIOutput() {
    size_t header_end = _output.find("\r\n\r\n");
    
    if (header_end == std::string::npos) {
        _output.clear();
        _status_code = 500;
        return;
    }
    
    std::string cgi_headers = _output.substr(0, header_end);
    std::string cgi_body = _output.substr(header_end + 4);
    
    std::istringstream header_stream(cgi_headers);
    std::string line;
    
    while (std::getline(header_stream, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }
        
        if (line.empty()) continue;
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            
            size_t start = value.find_first_not_of(" \t\r\n");
            size_t end = value.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                value = value.substr(start, end - start + 1);
            }
            
            if (key == "Status") {
                int status_code = std::atoi(value.c_str());
                if (status_code > 0) {
                    _status_code = status_code;
                }
            }
        }
    }
    
    _output = cgi_body;
}

bool CGI::execute() {
    if (!findScriptPath()) {
        return false;
    }
    if (!findCGIExecutor()) {
        return false;
    }
    setupEnvironment();
    if (!createPipes()) {
        _status_code = 500;
        return false;
    }
    bool success = executeCGI();
    closePipes();
    
    return success;
}