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
	if (_status_code >= 400)
		return;
	
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
		size_t max_body_size = static_cast<size_t>(strtol(max_body_size_str.c_str(), NULL, 10));
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
		std::string extension;
		size_t dot_pos = _request.getPath().find_last_of('.');
			if (dot_pos != std::string::npos)
				extension = _request.getPath().substr(dot_pos + 1);
		if (extension == "bla" && loc.getPath() == "~ \\.bla$" && _request.getMethod() == "POST")
		{
			target_location = loc;
			_request.setTargetLocation(target_location);
			return;
		}	

		size_t match_index = _request.getPath().find(loc.getPath());
		if (match_index != std::string::npos && 
		    (match_index + loc.getPath().length() == _request.getPath().length() || 
		     _request.getPath()[match_index + loc.getPath().length()] == '/') &&
		    size > max_match)
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
	std::ifstream file(file_path.c_str());
	std::cerr << "Reading file: " << file_path << std::endl;
	std::string extension;
	extension = file_path.substr(file_path.find_last_of('.') + 1);
	if (extension != "html")
	{
		_status_code = 200;
	}
	if (access(file_path.c_str(), R_OK) != 0)
	{
		_status_code = 404;
		_request.setValidRequest(false);
		return;
	}	
	_status_code = 200;
	std::ostringstream ss;
	std::ostringstream oss;
	if (!file.is_open())
	{
		_status_code = 404;
		_request.setValidRequest(false);
		return;
	}
	ss << file.rdbuf();
	_body = ss.str();
	_status_code = 200;
	oss << _body.size();
	_headers["Content-Length"] = oss.str();
	_headers["Content-Type"] = getMimeType(file_path);
	_headers["Connection"] = "close";
	file.close();
	return;
}

void HttpResponse::makeAutoindex()
{
	DIR *dir;
	struct dirent *entry;
	std::string html_page;
	std::string requested_path = _request.getPath();
	std::string location_path = _request.getTargetLocation().getPath();
	std::string root = _request.getTargetLocation().getDirective("root");
	
	if (root.empty())
		root = "";
	
	std::string relative_path = requested_path;
	if (!_request.getTargetLocation().getDirective("root").empty() && 
	    relative_path.find(location_path) == 0)
	{
		relative_path = relative_path.substr(location_path.length());
	}
	
	if (!relative_path.empty() && relative_path[0] == '/')
		relative_path = relative_path.substr(1);
	
	std::string current_path;
	if (root.empty())
		root = ".";
	
	if (root == "." || root == "./")
		current_path = root + (root[root.length()-1] == '/' ? "" : "/") + relative_path;
	else if (root == "/")
		current_path = "/" + relative_path;
	else
		current_path = root + "/" + relative_path;
	
	if (current_path.length() > 1 && current_path[current_path.length() - 1] == '/')
		current_path = current_path.substr(0, current_path.length() - 1);
		
	if ((dir = opendir(current_path.c_str())) != NULL)
	{
		std::string display_path = requested_path;
		if (display_path.length() > 1 && display_path[display_path.length() - 1] == '/')
			display_path = display_path.substr(0, display_path.length() - 1);
		
		html_page = "<html><head><title>Index of " + display_path + "</title></head><body>";
		html_page += "<h1>Index of " + display_path + "/</h1><ul>";
		
		while ((entry = readdir(dir)) != NULL)
		{
			std::string entry_name = entry->d_name;
			std::string entry_path;
			
			if (entry_name == ".")
			{
				entry_path = "./";
			}
			else if (entry_name == "..")
			{
				entry_path = "../";
			}
			else
			{
				std::string full_filesystem_path = current_path + "/" + entry_name;
				struct stat entry_stat;
				stat(full_filesystem_path.c_str(), &entry_stat);
				
				if (S_ISDIR(entry_stat.st_mode))
				{
					entry_path = entry_name + "/";
				}
				else
				{
					entry_path = entry_name;
				}
			}
			
			std::string url_path;
			if (entry_name == ".")
				url_path = "./";
			else if (entry_name == "..")
				url_path = "../";
			else
				url_path = entry_name;
			
			html_page += "<li><a href=\"" + url_path + "\">" + entry_path + "</a></li>";
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
	else if (_request.isValidRequest())
	{
		_status_code = 404;
		_request.setValidRequest(false);
		makeErrorResponse();
	}
	return;
}

void HttpResponse::handleCGI() {	
	CGI cgi(_request, _request.getTargetLocation());
	
	if (!cgi.execute()) {
		_status_code = cgi.getStatusCode();
		makeErrorResponse();
		return;
	}
	
	std::string cgi_output = cgi.getOutput();
	
	size_t header_end = cgi_output.find("\r\n\r\n");
	
	if (header_end != std::string::npos) {
		std::string cgi_headers = cgi_output.substr(0, header_end);
		std::string cgi_body = cgi_output.substr(header_end + 4);
		
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
				
				_headers[key] = value;
			}
		}
		
		_body = cgi_body;
		_status_code = 200;
		
		if (_headers.find("Content-Length") == _headers.end()) {
			std::ostringstream ss;
			ss << _body.size();
			_headers["Content-Length"] = ss.str();
		}
	} else {
		_body = cgi_output;
		_status_code = 200;
		_headers["Content-Type"] = "text/html";
		std::ostringstream ss;
		ss << _body.size();
		_headers["Content-Length"] = ss.str();
	}
	
	_reason = getReason();
}

void HttpResponse::handleGet(const ServerConfig &server_config)
{
	Location target_location = _request.getTargetLocation();

	if (!target_location.getDirective("rewrite").empty())
	{
		redirection(target_location.getDirective("rewrite"));
		makeErrorResponse();
		return;
	}
	std::string requested_path = _request.getPath();
	if (!requested_path.empty() && requested_path[requested_path.length() - 1] != '/')
	{
		std::string check_path;
		std::string root = target_location.getDirective("root");
		if (root.empty())
			root = target_location.getDirective("root");
		
		std::string relative_path = requested_path;
		if (!target_location.getDirective("root").empty() && 
		    relative_path.find(target_location.getPath()) == 0)
		{
			relative_path = relative_path.substr(target_location.getPath().length());
		}
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path = relative_path.substr(1);
		
		if (root == "./" || root == ".")
			check_path = root + (root[root.length()-1] == '/' ? "" : "/") + relative_path;
		else
			check_path = root + "/" + relative_path;
		struct stat path_stat;
		if (stat(check_path.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode))
		{
			_status_code = 301;
			_headers["Location"] = requested_path + "/";
			_reason = getReason();
			_body.clear();
			_headers["Content-Length"] = "0";
			return;
		}
	}
	
	if (_request.getFile().empty())
	{
		if (_request.getPath() != target_location.getPath())
		{
			std::string root = target_location.getDirective("root");
			if (root.empty())
				root = server_config.getDirective("root");
			
			std::string relative_path = _request.getPath();
			std::string location_path = target_location.getPath();
			if (!target_location.getDirective("root").empty() && 
			    relative_path.find(location_path) == 0)
			{
				relative_path = relative_path.substr(location_path.length());
			}
			
			if (!relative_path.empty() && relative_path[0] == '/')
				relative_path = relative_path.substr(1);
			
			std::string full_path;
			if (root == "./" || root == ".")
				full_path = root + (root[root.length()-1] == '/' ? "" : "/") + relative_path;
			else if (root.empty() || root == "/")
				full_path = "/" + relative_path;
			else
				full_path = root + "/" + relative_path;
			
			struct stat file_stat;
			if (stat(full_path.c_str(), &file_stat) != 0)
			{
				_status_code = 404;
				makeErrorResponse();
				return;
			}
		}
		
		if (target_location.getDirective("index") == "")
		{
			if (server_config.getDirective("index").empty() == false)
			{
				_request.setPathFile(server_config.getDirective("index"));
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
		std::string root = target_location.getDirective("root");
		
		if (root.empty())
			root = server_config.getDirective("root");
		
		std::string relative_path = _request.getPath();
		
		std::string location_path = target_location.getPath();
		if (!target_location.getDirective("root").empty() && 
		    relative_path.find(location_path) == 0)
		{
			relative_path = relative_path.substr(location_path.length());
		}
		
		if (!_request.getFile().empty())
		{
			size_t last_slash = relative_path.find_last_of('/');
			if (last_slash != std::string::npos)
				relative_path = relative_path.substr(0, last_slash + 1) + _request.getFile();
			else
				relative_path = _request.getFile();
		}
		
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path = relative_path.substr(1);
		
		if (root == "./" || root == ".")
			full_path = root + (root[root.length()-1] == '/' ? "" : "/") + relative_path;
		else if (root.empty() || root == "/")
			full_path = "/" + relative_path;
		else
			full_path = root + "/" + relative_path;
		
		readFile(full_path);
		if (_status_code == 404 || _status_code == 406)
		{
			if (target_location.getDirective("autoindex") == "on")
			{
				makeAutoindex();
				return;
			}
			else
			{
				makeErrorResponse();
				return;
			}
		}
		return;
	}
	else
	{
		handleCGI();
		return;
	}
}

void HttpResponse::handlePost()
{
	Location target_location = _request.getTargetLocation();
	
	if (target_location.getDirective("cgi_processing") == "on" || 
	    !target_location.getDirective("cgi_path").empty())
	{
		handleCGI();
		return;
	}

	if (_request.getBody().empty())
	{
		_status_code = 400;
		_request.setValidRequest(false);
		makeErrorResponse();
		return;
	}
	
	std::ostringstream filename_ss;
	filename_ss << "upload_" << time(NULL) << ".txt";
	std::string filename = filename_ss.str();
	std::string filepath;
	if (target_location.getDirective("upload_dir") != "")
	{
		std::string upload_path = target_location.getDirective("upload_dir");
		if (upload_path[upload_path.length() - 1] != '/')
			upload_path += "/";
		filepath = upload_path + filename;
	}
	else
		filepath = "./uploads/" + filename;

	std::ofstream file(filepath.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		_status_code = 500;
		_request.setValidRequest(false);
		makeErrorResponse();
		return;
	}
	file << _request.getBody();
	file.close();
	_status_code = 201;

	std::ostringstream body_ss;
	body_ss << "<html><body><h1>File uploaded successfully</h1>";
	body_ss << "<p>Filename: " << filename << "</p>";
	body_ss << "</body></html>";
	_body = body_ss.str();
	_headers["Content-Type"] = "text/html";
	std::ostringstream oss;
	oss << _body.size();
	_headers["Content-Length"] = oss.str();
	_headers["Connection"] = "close";
	return;
}

void HttpResponse::handleDelete(const ServerConfig &server_config)
{
	Location target_location = _request.getTargetLocation();
	
	if (!target_location.getDirective("rewrite").empty())
	{
		redirection(target_location.getDirective("rewrite"));
		makeErrorResponse();
		return;
	}

	std::string full_path;
	std::string root = target_location.getDirective("root");
	std::string relative_path = _request.getPath();
	
	if (root.empty())
		root = server_config.getDirective("root");
	
	std::string location_path = target_location.getPath();
	if (!target_location.getDirective("root").empty() && 
	    relative_path.find(location_path) == 0)
	{
		relative_path = relative_path.substr(location_path.length());
	}
	
	if (!relative_path.empty() && relative_path[0] == '/')
		relative_path = relative_path.substr(1);
	
	if (root.empty() || root == "/")
		full_path = "/" + relative_path;
	else
	{
		if (root.length() > 2 && root[root.length() - 1] == '/')
			root = root.substr(0, root.length() - 1);
		
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

	struct stat file_stat;
	if (stat(full_path.c_str(), &file_stat) != 0)
	{
		_status_code = 404;
		makeErrorResponse();
		return;
	}

	if (S_ISDIR(file_stat.st_mode))
	{
		_status_code = 403;
		makeErrorResponse();
		return;
	}

	if (access(full_path.c_str(), W_OK) != 0)
	{
		_status_code = 403;
		makeErrorResponse();
		return;
	}

	if (remove(full_path.c_str()) != 0)
	{
		_status_code = 500;
		makeErrorResponse();
		return;
	}

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
			break;
		}
			
	}
	handleTargetLocation(servers[server_index].getServerConfig());
	checkClientMaxBodySize();
	this->isAllowedMethod();
	if (_request.isValidRequest())
	{
		if (_request.getMethod() == "GET" || _request.getMethod() == "HEAD")
		{
			handleGet(servers[server_index].getServerConfig());
			if (_request.getMethod() == "HEAD")
			{
				_body = "";
			}
		}
		else if (_request.getMethod() == "POST")
		{
			handlePost();
		}
		else if (_request.getMethod() == "DELETE")
		{
			handleDelete(servers[server_index].getServerConfig());
		}
	}
	else
	{
		makeErrorResponse();
	}
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