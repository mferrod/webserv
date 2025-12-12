#include "../includes/HttpResponse.hpp"
#include <unistd.h>

HttpResponse::HttpResponse()
{
	_status_code = 0;
	_reason = "";
	_headers.clear();
	_body = "";
}

HttpResponse::~HttpResponse() {}

HttpResponse::HttpResponse(const HttpResponse &other)
{
	_status_code = other._status_code;
	_reason = other._reason;
	_headers = other._headers;
	_body = other._body;
	_request = other._request;
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other)
{
	if (this != &other)
	{
		_status_code = other._status_code;
		_reason = other._reason;
		_headers = other._headers;
		_body = other._body;
		_request = other._request;
	}
	return *this;
}

HttpResponse::HttpResponse(const HttpRequest &req)
{
	_status_code = req.getStatusCode();
	_reason = "";
	_headers.clear();
	_body = "";
	_request = req;
}

void HttpResponse::isAllowedMethod()
{
	std::string allowed_methods = _request.getTargetLocation().getDirective("allowed_methods");
	if (allowed_methods.find(_request.getMethod()) == std::string::npos)
	{
		_status_code = 405;
		_request.setValidRequest(false);
		return;
	}
}

void HttpResponse::checkClientMaxBodySize()
{
	std::string max_body_size_str = _request.getTargetLocation().getDirective("client_max_body_size");
	if (!max_body_size_str.empty())
	{
		size_t max_body_size = static_cast<size_t>(strtol(max_body_size_str.c_str(), NULL, 10)); // Convert value to size_t
		if (_request.getBody().size() > max_body_size)
		{
			_status_code = 413;
			_request.setValidRequest(false);
			return;
		}
	}
}

void HttpResponse::handleTargetLocation(const ServerConfig &server_config)
{
	size_t max_match = 0;
	Location target_location = server_config.getLocations()[0];
	for (size_t i = 1; i < server_config.getLocationCount(); i++)
	{
		Location loc = server_config.getLocations()[i];
		size_t size = 0;
		for (size_t j = 0; j < loc.getPath().length(); j++)
		{
			if (loc.getPath()[j] == '/' && loc.getPath()[j + 1] != '\0')
				size++;
		}
		size_t match_index = _request.getPath().find(loc.getPath());
		if (match_index != std::string::npos && (_request.getPath()[match_index + loc.getPath().length()] == '/' || match_index + loc.getPath().length() == '\0')
		&& size > max_match)
		{
			max_match = size;
			target_location = loc;
		}
	}
	_request.setTargetLocation(target_location);
}

void HttpResponse::redirection(std::string url)
{
	_headers.clear();
	_body.clear();
	_headers["Location"] = url;
	_status_code = 301;
}

void HttpResponse::readFile(const std::string &file_path)
{
	//std::cout << "Attempting to read file: " << file_path << std::endl;
	std::ifstream file(file_path.c_str());
	//std::cerr << "Reading file: " << file_path << std::endl;
	if (!file.is_open())
	{
		_status_code = 404;
		_request.setValidRequest(false);
		return;
	}
	std::ostringstream ss;
	std::ostringstream oss;

	ss << file.rdbuf();
	_body = ss.str();
	_status_code = 200;
	oss << _body.size();
	_headers["Content-Length"] = oss.str();
	_headers["Content-Type"] = getMimeType(file_path);
	return;
}

void HttpResponse::makeAutoindex()
{
	DIR *dir;
	struct dirent *entry;
	std::string html_page;
	std::string path = _request.getTargetLocation().getPath();
	std::string current_path = _request.getTargetLocation().getDirective("root") + "/" + _request.getPath().substr(_request.getTargetLocation().getPath().length());
	std::string root_path = _request.getTargetLocation().getDirective("root");

	if (path[path.length() - 1] == '/')
		path = path.erase(path.length() - 1);
	if (current_path[current_path.length() - 1] == '/')
		current_path = current_path.erase(current_path.length() - 1);
	if ((dir = opendir(current_path.c_str())) != NULL)
	{
		html_page = "<html><head><title>Index of " + path + "</title></head><body>";
		std::string full_path = path + "/" + current_path.substr(root_path.length());
		if (full_path[full_path.length() - 1] == '/')
			full_path = full_path.erase(full_path.length() - 1);
		html_page += "<h1>Index of " + full_path + "</h1><ul>";
		
		while ((entry = readdir(dir)) != NULL)
		{
			std::string entry_name = entry->d_name;
			if (entry_name != "." && entry_name != "..")
				full_path  += "/" + entry_name;
			else
				full_path = entry_name;

			struct stat	entry_stat;
			stat(full_path.c_str(), &entry_stat);
			if (S_ISDIR(entry_stat.st_mode) && entry->d_type != DT_REG)
				html_page += "<li><a href=\"" + full_path + "\">" + "\t" +entry_name + "/" + "</a></li>";
			else if (entry->d_type == DT_REG)
			{
				std::string extension;
				size_t dot_pos = entry_name.find_last_of('.');
				if (dot_pos != std::string::npos)
					extension = entry_name.substr(dot_pos + 1);
				if (extension == "html") // Add more file types if implemented after cgi implementation
					html_page += "<li><a href=\"" + full_path + "\">" + "    " + entry_name + "</a></li>";
				else
					html_page += "<li>    " + entry_name + "</li>";
			}	

		}
		html_page += "</ul></body></html>";
		closedir(dir);
		_status_code = 200;
		_body.clear();
		_body = html_page;
		_headers["Content-Type"] = "text/html";
		std::ostringstream oss;
		oss << _body.size();
		_headers["Content-Length"] = oss.str();
	}
	else
	{
		_status_code = 404;
		makeErrorResponse();
	}
}

void HttpResponse::handleCGI() {
	std::cout << "\n[Response] Procesando request CGI..." << std::endl;
	
	// Crear objeto CGI con el request y la location
	CGI cgi(_request, _request.getTargetLocation());
	
	// Ejecutar el CGI
	if (!cgi.execute()) {
		std::cerr << "[Response] Error ejecutando CGI" << std::endl;
		_status_code = cgi.getStatusCode();
		makeErrorResponse();
		return;
	}
	
	// Obtener la salida del CGI
	std::string cgi_output = cgi.getOutput();
	
	// Verificar si el CGI devolvió headers propios
	size_t header_end = cgi_output.find("\r\n\r\n");
	
	if (header_end != std::string::npos) {
		// El CGI generó headers completos (Content-Type, etc)
		std::string cgi_headers = cgi_output.substr(0, header_end);
		std::string cgi_body = cgi_output.substr(header_end + 4);
		
		// Parsear los headers del CGI
		std::istringstream header_stream(cgi_headers);
		std::string line;
		
		while (std::getline(header_stream, line)) {
			// Eliminar \r si existe
			if (!line.empty() && line[line.length() - 1] == '\r') {
				line = line.substr(0, line.length() - 1);
			}
			
			if (line.empty()) continue;
			
			size_t colon = line.find(':');
			if (colon != std::string::npos) {
				std::string key = line.substr(0, colon);
				std::string value = line.substr(colon + 1);
				
				// Trim spaces
				size_t start = value.find_first_not_of(" \t\r\n");
				size_t end = value.find_last_not_of(" \t\r\n");
				if (start != std::string::npos) {
					value = value.substr(start, end - start + 1);
				}
				
				_headers[key] = value;
			}
		}
		
		_body = cgi_body;
		_status_code = 200;
		
		// Asegurar que tenemos Content-Length
		if (_headers.find("Content-Length") == _headers.end()) {
			std::ostringstream ss;
			ss << _body.size();
			_headers["Content-Length"] = ss.str();
		}
	} else {
		// El CGI solo devolvió el body, añadir headers nosotros
		_body = cgi_output;
		_status_code = 200;
		_headers["Content-Type"] = "text/html";
		std::ostringstream ss;
		ss << _body.size();
		_headers["Content-Length"] = ss.str();
	}
	
	_reason = getReason();
	std::cout << "[Response] CGI procesado exitosamente" << std::endl;
}

void HttpResponse::handleGet(const ServerConfig &server_config)
{
	Location target_location = _request.getTargetLocation();
	
	//comprobar si tiene cgi_path
	if (!target_location.getDirective("cgi_path").empty()) {
		std::cout << "[Response] Detectado CGI por cgi_path presente" << std::endl;
		handleCGI();
		return;
	}

	if (!target_location.getDirective("rewrite").empty())
	{
		redirection(target_location.getDirective("rewrite"));
		makeErrorResponse();
		return;
	}

	if (_request.getFile().empty())
	{
		if (target_location.getDirective("index") == "")
		{
			if (server_config.getDirective("index").empty() == false)
			{
				_request.setPathFile(server_config.getDirective("index"));
				//std::cout << "Using server index file: " << server_config.getDirective("index") << std::endl;
				//std::cout << "Request path file: " << _request.getFile() << std::endl;
			}

			else
			{
				_status_code = 404;
				makeErrorResponse();
				return;
			}
		}
		else if (_request.getPath() == target_location.getPath() && target_location.getDirective("index") != "")
		{
			_request.setPathFile(target_location.getDirective("index"));
		}
		else if (target_location.getDirective("autoindex") == "on")
		{
			makeAutoindex();
			return;
		}
		else
		{
			_status_code = 404;
			makeErrorResponse();
			return;
		}
	}

	if (target_location.getDirective("cgi_processing") == "")
	{
		std::string full_path;
		//std::cout << "Server root directive: " << server_config.getDirective("root") << std::endl;
		if (server_config.getDirective("root") == "./") // Intento de solucionar problema con ruta
		{
			full_path = _request.getFile();
			//std::cout << "Server root is '/', using file path directly: " << full_path << std::endl;
		}	
		else
			full_path = target_location.getDirective("root") + "/" + _request.getPath().substr(target_location.getPath().length()) + "/" + _request.getFile();
		readFile(full_path);
		if (_status_code == 404)
		{
			if (target_location.getDirective("autoindex") == "on")
			{
				makeAutoindex();
				return;
			}
			else
				makeErrorResponse();
		}
		return;
	}
	/* else
		cgiExec(); // Not implemented yet */

		

	/* struct stat file_stat;
	// Check if file exists
	if (stat(file_path.c_str(), &file_stat) < 0)
	{
		_status_code = 404;
		makeErrorResponse();
		return;
	}
	// Check if it's a regular file
	if (!S_ISREG(file_stat.st_mode))
	{
		_status_code = 403;
		makeErrorResponse();
		return;
	}

	// Open and read file
	std::ifstream file(file_path.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		_status_code = 500;
		makeErrorResponse();
		return;
	}
	std::string body;
	body.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	_body = body;
	_status_code = 200;
	_headers["Content-Length"] = std::to_string(_body.size());
	_headers["Content-Type"] = getMimeType(file_path); */
}

void HttpResponse::handleDelete(const ServerConfig &server_config)
{
	Location target_location = _request.getTargetLocation();
	
	// Check if redirection is configured
	if (!target_location.getDirective("rewrite").empty())
	{
		redirection(target_location.getDirective("rewrite"));
		makeErrorResponse();
		return;
	}

	// Construir la ruta completa al recurso
	std::string full_path;
	std::string root = target_location.getDirective("root");
	std::string relative_path = _request.getPath();
	
	// Si el root de la location está vacío, usar el root del servidor
	if (root.empty())
		root = server_config.getDirective("root");
	
	// Eliminar la barra inicial del path si está presente
	if (!relative_path.empty() && relative_path[0] == '/')
		relative_path = relative_path.substr(1);
	
	// Manejar la ruta root
	if (root.empty() || root == "/")
		full_path = "/" + relative_path;
	else
	{
		// Eliminar barra final del root (pero mantener "./" tal cual)
		if (root.length() > 2 && root[root.length() - 1] == '/')
			root = root.substr(0, root.length() - 1);
		
		// Construir la ruta completa al recurso
		if (!relative_path.empty())
		{
			if (root == ".")
				full_path = "./" + relative_path;
			else if (root == "./")
				full_path = root + relative_path;
			else
				full_path = root + "/" + relative_path;
		}
		else
			full_path = root;
	}

	// Verificamos si el recurso existe
	struct stat file_stat;
	if (stat(full_path.c_str(), &file_stat) != 0)
	{
		_status_code = 404;
		makeErrorResponse();
		return;
	}

	// Verificamos si es un directorio
	if (S_ISDIR(file_stat.st_mode))
	{
		// Devolvemos 403 Forbidden para directorios
		_status_code = 403;
		makeErrorResponse();
		return;
	}

	// Verificamos si tenemos permisos de escritura
	if (access(full_path.c_str(), W_OK) != 0)
	{
		_status_code = 403;
		makeErrorResponse();
		return;
	}

	// Intentar eliminar el archivo
	if (remove(full_path.c_str()) != 0)
	{
		// Si al borrarfalla, devolver 500
		_status_code = 500;
		makeErrorResponse();
		return;
	}

	// Archivo eliminado correctamente
	// Devolver 204 No Content (estándar para DELETE exitoso sin cuerpo de respuesta)
	_status_code = 204;
	_reason = getReason();
	_body.clear();
	_headers["Content-Length"] = "0";
	_headers["Connection"] = "close";
}

void HttpResponse::handleRequest(std::vector<ServerSocket> &servers)
{
	size_t server_index;
	for (server_index = 0; server_index < servers.size(); server_index++)
	{
		int port;
		std::stringstream ss(servers[server_index].getServerConfig().getDirective("listen"));
		ss >> port;
		if (servers[server_index].getPort() == port)
		{
			/* if (!servers[server_index].getServerConfig().getDirective("server_name").empty() 
			&& servers[server_index].getServerConfig().getDirective("server_name") != _request.getHost())
				continue; */
			//if //Check client ip against host configuration
			break;
		}
			
	}
	handleTargetLocation(servers[server_index].getServerConfig());
	checkClientMaxBodySize();
	isAllowedMethod();
	if (_request.isValidRequest())
	{
		if (_request.getMethod() == "GET") // Check method implementation
		{
			handleGet(servers[server_index].getServerConfig());
		}
		/* else if (_request.getMethod() == "POST")
		{
			handlePost();
		} */
		else if (_request.getMethod() == "DELETE")
		{
			handleDelete(servers[server_index].getServerConfig());
		}


		/* else //Necessary?
		{
			makeErrorResponse();
		} */
	}
	else
	{
		makeErrorResponse();
	}
	//std::cout << "Response built with status code: " << _status_code << std::endl;
	//std::cout << "Response body size: " << _body.size() << " bytes" << std::endl;
	//std::cout << "Response body: " << std::endl << _body << std::endl;
}

std::string HttpResponse::getReason()
{
	switch (_status_code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 204: return "No Content";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 417: return "Expectation Failed";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		case 507: return "Insufficient Storage";
		default: return "Internal Server Error";
	}
}

void HttpResponse::makeErrorResponse()
{
	// If _status_code is already set (e.g., by handleDelete), use it
	// Otherwise, get it from the request
	if (_status_code == 0 || _status_code == 200)
		_status_code = _request.getStatusCode();
	_reason = getReason();
	std::ostringstream ss;
	ss << _status_code;
	_body = "<html><body><h1>" + ss.str() + " " + _reason + "</h1></body></html>";
	_headers["Content-Type"] = "text/html";
	std::ostringstream oss;
	oss << _body.size();
	_headers["Content-Length"] = oss.str();
	_headers["Connection"] = "close";
	return;
}

std::string HttpResponse::buildResponse()
{
	std::string response;

	std::ostringstream ss;
	ss << _status_code;
	response += "HTTP/1.1 " + ss.str() + " " + _reason + "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response += it->first + ": " + it->second + "\r\n";
	}

	response += "\r\n";
	response += _body;

	return response;
}