#include "../includes/HttpResponse.hpp"

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
	std::ifstream file(file_path.c_str());
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

void HttpResponse::handleGet()
{
	Location target_location = _request.getTargetLocation();
	if (!target_location.getDirective("rewrite").empty())
	{
		redirection(target_location.getDirective("rewrite"));
		makeErrorResponse();
		return;
	}

	if (_request.getFile().empty())
	{
		if (_request.getPath() == target_location.getPath() && target_location.getDirective("index") != "")
			_request.setPathFile(target_location.getDirective("index"));
		else if (target_location.getDirective("autoindex") == "on") // Simple autoindex implementation
		{
			_status_code = 200;
			_body = "<html><body><h1>Autoindex is ON - Directory listing for " + _request.getPath() + "</h1></body></html>";
			_headers["Content-Type"] = "text/html";
			std::ostringstream oss;
			oss << _body.size();
			_headers["Content-Length"] = oss.str();
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
		std::string full_path = target_location.getDirective("root") + "/" + _request.getPath().substr(target_location.getPath().length()) + "/" + _request.getFile();
		readFile(full_path);
		if (_status_code == 404)
		{
			if (target_location.getDirective("autoindex") == "on") // Simple autoindex implementation
			{
				_status_code = 200;
				_body = "<html><body><h1>Autoindex is ON - Directory listing for " + _request.getPath() + "</h1></body></html>";
				_headers["Content-Type"] = "text/html";
				std::ostringstream oss;
				oss << _body.size();
				_headers["Content-Length"] = oss.str();
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

void HttpResponse::handleRequest(std::vector<ServerSocket> &servers)
{
	size_t server_index;
	for (server_index = 0; server_index < servers.size(); server_index++)
	{
		int port;
		std::stringstream ss(servers[server_index].getServerConfig().getDirective("port"));
		ss >> port;
		if (servers[server_index].getPort() == port)
		{
			if (!servers[server_index].getServerConfig().getDirective("server_name").empty() 
			&& servers[server_index].getServerConfig().getDirective("server_name") != _request.getHost())
				continue;
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
			handleGet();
		}
		//Pending implementation
		/* else if (_request.getMethod() == "POST")
		{
			handlePost();
		}
		else if (_request.getMethod() == "DELETE")
		{
			handleDelete();
		} */


		/* else //Necessary?
		{
			makeErrorResponse();
		} */
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