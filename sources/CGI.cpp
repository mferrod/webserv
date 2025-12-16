#include "../includes/CGI.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
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

// Encuentra el archivo del script a ejecutar
bool CGI::findScriptPath() {
    // El path completo sería: root + path del request
    std::string root = _location.getDirective("root");
    std::string request_path = _request.getPath();
    
    // Si location es /cgi-bin y request es /cgi-bin/script.py
    // Entonces: root/script.py
    std::string location_path = _location.getPath();
    if (request_path.find(location_path) == 0) {
        request_path = request_path.substr(location_path.length());
    }
    
    _script_path = _location.getDirective("cgi_path");
    std::cout << "CGI: Script path determined as: " << _script_path << std::endl;
    
    // Verificar que existe
    if (access(_script_path.c_str(), F_OK) != 0) {
        std::cerr << "CGI: Script no encontrado: " << _script_path << std::endl;
        _status_code = 404;
        return false;
    }
    
    // Verificar que es ejecutable (opcional)
    /* if (access(_script_path.c_str(), X_OK) != 0) {
        std::cerr << "CGI: Script sin permisos de ejecución: " << _script_path << std::endl;
        _status_code = 403;
        return false;
    } */
    
    return true;
}

// Encuentra el intérprete (python, bash, php...)
bool CGI::findCGIExecutor() {
    // Obtener extensión del archivo
    size_t dot_pos = _location.getPath().find_last_of('.');
    if (dot_pos == std::string::npos) {
        std::cerr << "CGI: No se puede determinar tipo de script" << std::endl;
        _status_code = 500;
        return false;
    }
    
    std::string extension = _location.getPath().substr(dot_pos);
    if (extension[extension.size() - 1] == '$')
        extension = extension.substr(0, extension.size() - 1);
    std::cout << "CGI: Script extension: " << extension << std::endl;
    // Buscar en cgi_path de la configuración
    std::string cgi_path = _location.getDirective("cgi_path");
    std::string cgi_ext = _location.getDirective("cgi_ext");
    
    if (cgi_path.empty()) {
        std::cerr << "CGI: cgi_path no configurado" << std::endl;
        _status_code = 500;
        return false;
    }
    
    // Verificar si la extensión está permitida
    if (!cgi_ext.empty() && cgi_ext.find(extension) == std::string::npos) {
        std::cerr << "CGI: Extensión no permitida: " << extension << std::endl;
        _status_code = 403;
        return false;
    }
    
    // Mapeo de extensiones a intérpretes
    // En tu config tienes: cgi_path /usr/bin/python3 /bin/bash
    // Y cgi_ext .py .sh
    
    if (extension == ".py") {
        // Buscar python3 en cgi_path
        if (cgi_path.find("python") != std::string::npos) {
            size_t start = cgi_path.find("python");
            size_t end = cgi_path.find(" ", start);
            if (end == std::string::npos) end = cgi_path.length();
            
            // Retroceder para obtener la ruta completa
            while (start > 0 && cgi_path[start - 1] != ' ') start--;
            _cgi_executor = cgi_path.substr(start, end - start);
        }
    }
    else if (extension == ".sh") {
        if (cgi_path.find("bash") != std::string::npos) {
            size_t start = cgi_path.find("bash");
            size_t end = cgi_path.find(" ", start);
            if (end == std::string::npos) end = cgi_path.length();
            
            while (start > 0 && cgi_path[start - 1] != ' ') start--;
            _cgi_executor = cgi_path.substr(start, end - start);
        }
    }
    else if (extension == ".php") {
        if (cgi_path.find("php") != std::string::npos) {
            size_t start = cgi_path.find("php");
            size_t end = cgi_path.find(" ", start);
            if (end == std::string::npos) end = cgi_path.length();
            
            while (start > 0 && cgi_path[start - 1] != ' ') start--;
            _cgi_executor = cgi_path.substr(start, end - start);
        }
    }
    else if (extension == ".bla")
        _cgi_executor = "./ubuntu_cgi_tester"; // Specific case for .bla files
    
    if (_cgi_executor.empty()) {
        std::cerr << "CGI: No se encontró intérprete para " << extension << std::endl;
        _status_code = 500;
        return false;
    }
    
    // Verificar que el intérprete existe
    if (access(_cgi_executor.c_str(), X_OK) != 0) {
        std::cerr << "CGI: Intérprete no ejecutable: " << _cgi_executor << std::endl;
        _status_code = 500;
        return false;
    }
    
    return true;
}

// Prepara todas las variables de entorno para el CGI
void CGI::setupEnvironment() {
    // REQUEST_METHOD
    _env_vars["REQUEST_METHOD"] = _request.getMethod();
    
    // QUERY_STRING (parte después del ?)
    std::string path = _request.getPath();
    size_t query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        _env_vars["QUERY_STRING"] = path.substr(query_pos + 1);
    } else {
        _env_vars["QUERY_STRING"] = "";
    }
    
    // SCRIPT_NAME (el path hasta el script, sin query string)
    std::string script_name = _request.getPath();
    size_t query_pos_script = script_name.find('?');
    if (query_pos_script != std::string::npos) {
        script_name = script_name.substr(0, query_pos_script);
    }
    _env_vars["SCRIPT_NAME"] = script_name;
    
    // PATH_INFO (parte del path después del nombre del script)
    // Por ejemplo: /cgi-bin/script.py/extra/info
    // SCRIPT_NAME = /cgi-bin/script.py
    // PATH_INFO = /extra/info
    std::string location_path = _location.getPath();
    std::string request_path_clean = script_name;
    
    // Quitar el location path del inicio
    if (request_path_clean.find(location_path) == 0) {
        request_path_clean = request_path_clean.substr(location_path.length());
    }
    
    // Buscar el nombre del script en el path
    size_t script_name_end = request_path_clean.find_first_of('?', 1);
    if (script_name_end != std::string::npos) {
        // Hay algo después del script
        _env_vars["PATH_INFO"] = request_path_clean.substr(script_name_end);
    } else {
        _env_vars["PATH_INFO"] = "";
    }
    
    // SERVER_PROTOCOL
    _env_vars["SERVER_PROTOCOL"] = "HTTP/1.1";
    
    // SERVER_NAME y SERVER_PORT
    _env_vars["SERVER_NAME"] = _request.getHost();
    std::ostringstream port_ss;
    port_ss << _request.getPort();
    _env_vars["SERVER_PORT"] = port_ss.str();
    
    // CONTENT_LENGTH y CONTENT_TYPE (para POST)
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
    
    // HTTP_* headers
    std::map<std::string, std::string> headers = _request.getHeaders();
    for (std::map<std::string, std::string>::iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string key = "HTTP_" + it->first;
        // Convertir a mayúsculas y reemplazar - por _
        for (size_t i = 0; i < key.length(); i++) {
            if (key[i] == '-') key[i] = '_';
            key[i] = toupper(key[i]);
        }
        _env_vars[key] = it->second;
    }
    
    // REMOTE_ADDR (IP del cliente) - necesitarías obtenerla del socket
    _env_vars["REMOTE_ADDR"] = "127.0.0.1"; // Placeholder
    // Debug: mostrar todas las variables de entorno
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
        // ═══════════════════════════════════════
        //           PROCESO HIJO (CGI)
        // ═══════════════════════════════════════
        
        // 1. Redirigir STDIN al pipe de entrada
        dup2(_pipe_in[0], STDIN_FILENO);
        close(_pipe_in[0]);
        close(_pipe_in[1]);
        
        // 2. Redirigir STDOUT al pipe de salida
        dup2(_pipe_out[1], STDOUT_FILENO);
        close(_pipe_out[0]);
        close(_pipe_out[1]);
        
        // 3. Preparar argv
        char *argv[3];
        argv[0] = const_cast<char*>(_cgi_executor.c_str());
        argv[1] = const_cast<char*>(_script_path.c_str());
        argv[2] = NULL;
        
        // 4. Preparar envp (variables de entorno)
        std::vector<std::string> env_strings;
        for (std::map<std::string, std::string>::iterator it = _env_vars.begin();
             it != _env_vars.end(); ++it) {
            env_strings.push_back(it->first + "=" + it->second);
        }
        
        char **envp = new char*[env_strings.size() + 1];
        for (size_t i = 0; i < env_strings.size(); i++) {
            envp[i] = const_cast<char*>(env_strings[i].c_str());
        }
        envp[env_strings.size()] = NULL;
        
        // 5. Ejecutar el CGI
        execve(argv[0], argv, envp);
        
        // Si llegamos aquí, execve falló
        std::cerr << "CGI: Error en execve(): " << strerror(errno) << std::endl;
        exit(1);
    }
    else {
        // ═══════════════════════════════════════
        //          PROCESO PADRE (Webserv)
        // ═══════════════════════════════════════
        
        // Cerrar extremos no usados de los pipes
        close(_pipe_in[0]);  // No leemos de stdin
        close(_pipe_out[1]); // No escribimos a stdout
        
        // 1. Escribir el body al CGI (si es POST)
        if (!writeRequestBody()) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            _status_code = 500;
            return false;
        }
        
        // Cerrar escritura para que el CGI vea EOF
        close(_pipe_in[1]);
        _pipe_in[1] = -1;
        
        // 2. Leer la salida del CGI
        if (!readCGIOutput()) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            _status_code = 500;
            return false;
        }
        
        // 3. Esperar a que termine el CGI
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::cerr << "CGI: El script terminó con error: " 
                      << WEXITSTATUS(status) << std::endl;
            _status_code = 500;
            return false;
        }
        
        // 4. Parsear la salida del CGI
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
    // El CGI puede devolver:
    // 1. Headers + Body (formato HTTP completo)
    // 2. Solo Body (y nosotros añadimos headers)
    
    // Buscar el separador de headers/body
    size_t header_end = _output.find("\r\n\r\n");
    
    if (header_end == std::string::npos) {
        // No hay headers, toda la salida es el body
        // El CGI debe generar headers, pero por si acaso...
        return;
    }
    
    // Si hay headers, el CGI ya generó una respuesta HTTP válida
    // En ese caso, dejamos _output tal cual está
    // porque será enviado directamente al cliente
}

// Método principal de ejecución
bool CGI::execute() {
    // 1. Encontrar el script
    if (!findScriptPath()) {
        return false;
    }
    
    // 2. Encontrar el intérprete
    if (!findCGIExecutor()) {
        return false;
    }
    
    // 3. Preparar entorno
    setupEnvironment();
    
    // 4. Crear pipes
    if (!createPipes()) {
        _status_code = 500;
        return false;
    }
    
    // 5. Ejecutar CGI
    bool success = executeCGI();
    
    // 6. Limpiar
    closePipes();
    
    return success;
}