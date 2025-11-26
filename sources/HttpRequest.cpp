#include "../includes/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	_method = "";
	_path = "";
	_path_query = "";
	_path_fragment = "";
	_version = "";
	_headers.clear();
	_body = "";
	_status_code = 0;
	_valid_request = true;
}

HttpRequest::~HttpRequest() {}

HttpRequest::HttpRequest(const HttpRequest &other)
{
	_method = other._method;
	_path = other._path;
	_path_query = other._path_query;
	_path_fragment = other._path_fragment;
	_version = other._version;
	_headers = other._headers;
	_body = other._body;
	_valid_request = other._valid_request;
	_status_code = other._status_code;
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other)
{
	if (this != &other)
	{
		_method = other._method;
		_path = other._path;
		_path_query = other._path_query;
		_path_fragment = other._path_fragment;
		_version = other._version;
		_headers = other._headers;
		_body = other._body;
		_valid_request = other._valid_request;
		_status_code = other._status_code;
	}
	return *this;
}

bool HttpRequest::isValidMethod(const std::string &method) const
{
	const std::string valid_methods[] = {
		"GET", "HEAD", "POST", "PUT", "DELETE", 
		"CONNECT", "OPTIONS", "TRACE", "PATCH"
	};
	
	for (size_t i = 0; i < sizeof(valid_methods) / sizeof(valid_methods[0]); i++)
	{
		if (method == valid_methods[i])
			return true;
	}
	return false;
}

bool HttpRequest::isImplementedMethod(const std::string &method) const
{
	// Check config file for implemented methods
	const std::string implemented_methods[] = {
		"GET", "POST", "DELETE"
	};

	for (size_t i = 0; i < sizeof(implemented_methods) / sizeof(implemented_methods[0]); i++)
	{
		if (method == implemented_methods[i])
			return true;
	}
	return false;
}

bool HttpRequest::isAllowedCharPath(unsigned char c) const
{
	if ((c >= '#' && c <= ';') || (c >= '?' && c <= '[') || (c >= 'a' && c <= 'z') ||
		c == '!' || c == '=' || c == ']' || c == '_' || c == '~')
		return (true);
	return (false);
}

bool HttpRequest::isValidPath(const std::string &path)
{
	if (path.empty())
	{
		_status_code = 400;
		std::cout << "Invalid request: empty path." << std::endl; // For debugging
		return false;
	}		
	
	if (path[0] != '/')
	{
		_status_code = 400;
		std::cout << "Invalid request: path must start with '/'" << std::endl; // For debugging
		return false;
	}
	
	if (path.length() > 8192)
	{
		_status_code = 414;
		std::cout << "Invalid request: path length exceeds 8192 characters." << std::endl; // For debugging
		return false;
	}
	
	for (size_t i = 1; i < path.length(); i++)
	{
		unsigned char c = path[i];
		
		if (!isAllowedCharPath(c))
		{
			_status_code = 400;
			std::cout << "Invalid request: disallowed character in path." << std::endl; // For debugging
			return false;
		}
	}
	
	if (path.find("../") != std::string::npos || 
		path.find("/..") != std::string::npos ||
		path == ".." || 
		path.find("/../") != std::string::npos)
	{
		_status_code = 400;
		std::cout << "Invalid request: path traversal detected." << std::endl; // For debugging
		return false;
	}
	
	if (path.find("//") != std::string::npos)
	{
		_status_code = 400;
		std::cout << "Invalid request: consecutive slashes detected." << std::endl; // For debugging
		return false;
	}

	return true;
}

// Decodes percent-encoded characters in the path
std::string HttpRequest::normalizePath(const std::string &path) const
{
	std::string normalized = path;
	
	size_t pos = 0;
	while ((pos = normalized.find('%', pos)) != std::string::npos)
	{
		if (pos + 2 < normalized.length())
		{
			std::string hex = normalized.substr(pos + 1, 2);
			unsigned char c = static_cast<unsigned char>(strtol(hex.c_str(), NULL, 16));

			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
				(c >= '0' && c <= '9') || c == '-' || c == '_' || 
				c == '.' || c == '~' || c == '/')
			{
				normalized.replace(pos, 3, 1, c);
			}
		}
		pos++;
	}
	
	return normalized;
}

bool HttpRequest::isValidVersion(const std::string &version)
{
	try
	{
		if (version.size() < 8 || version.substr(0, 5) != "HTTP/")
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: HTTP version must start with 'HTTP/'" << std::endl; // For debugging
			return false;
		}

		std::string num = version.substr(5);

		if (num.size() < 3 || num.size() > 3 || num[1] != '.' || !isdigit(num[0]) || !isdigit(num[2]))
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: HTTP version must be in the format 'HTTP/x.y'" << std::endl; // For debugging
			return false;
		}

		size_t dot = num.find('.');

		if (dot == std::string::npos)
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: HTTP version must be in the format 'HTTP/x.y'" << std::endl; // For debugging
			return false;
		}

		int major = strtol(num.substr(0, dot).c_str(), NULL, 10);
		int minor = strtol(num.substr(dot + 1).c_str(), NULL, 10);

		if (major == 0 && minor == 9)
			return true;
		else if (major == 1 && (minor == 0 || minor == 1))
			return true;
		else
		{
			_valid_request = false;
			_status_code = 505; // HTTP Version Not Supported
			std::cout << "Invalid request: HTTP version not supported." << std::endl; // For debugging
			return false;
		}
	}
	catch (const std::exception &e)
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: exception parsing HTTP version: " << e.what() << std::endl; // For debugging
		return false;
	}
}

bool HttpRequest::isValidHeaderValue(const std::string &value) const
{
	for (size_t i = 0; i < value.size(); i++)
	{
		char c = value[i];
		if (c < 32 && c != 9)
			return false;
	}
	return true;
}

bool HttpRequest::isValidHeader(const std::string &name, const std::string &value) const
{
	if (name.empty() || value.empty())
		return false;
	if (_headers.find("content-length") != _headers.end() && caseInsensitiveCompare(name, "Content-Length"))
		return false;
	if (!isToken(name))
		return false;
	if (!isValidHeaderValue(value))
		return false;
	return true;
}

void HttpRequest::parseRequestLine(const std::string &line)
{
	size_t method_end = line.find(' ');
	if (method_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: no space found in request line." << std::endl; // For debugging
		return;
	}
	_method = line.substr(0, method_end);

	if (!isValidMethod(_method))
	{
		_valid_request = false;
		_status_code = 400;  // Invalid method
		std::cout << "Invalid request: method not allowed." << std::endl; // For debugging
		return;
	}

	if (!isImplementedMethod(_method))
	{
		_valid_request = false;
		_status_code = 501;  // Not implemented method
		std::cout << "Invalid request: method not implemented." << std::endl; // For debugging
		return;
	}
	
	size_t path_end = line.find(' ', method_end + 1);
	if (path_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: no second space found in request line." << std::endl; // For debugging
		return;
	}

	std::string full_path = line.substr(method_end + 1, path_end - method_end - 1);

	full_path = normalizePath(full_path);

	if (!isValidPath(full_path))
	{
		_valid_request = false;
		std::cout << "Invalid request: " << _status_code << std::endl; // For debugging
		return;
	}

	if (full_path.find('?') != std::string::npos)
	{
		_path = full_path.substr(0, full_path.find('?'));
		_path_query = full_path.substr(full_path.find('?') + 1);
	}
	else if (full_path.find('#') != std::string::npos) // Handle fragments, ignore it or throw an error?
	{
		_path = full_path.substr(0, full_path.find('#'));
		_path_fragment = full_path.substr(full_path.find('#') + 1);
	}
	else
		_path = full_path;
	
	if (path_end + 1 >= line.size())
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: no HTTP version found in request line." << std::endl; // For debugging
		return;
	}
	_version = line.substr(path_end + 1);
	return;
}

void HttpRequest::getPort()
{
	if (_headers.find("host") != _headers.end())
	{
		std::string host_header = _headers["host"];
		size_t colon_pos = host_header.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string port_str = host_header.substr(colon_pos + 1);
			try
			{
				_port = static_cast<int>(strtol(port_str.c_str(), NULL, 10));
			}
			catch (const std::exception &e)
			{
				std::cerr << "Exception parsing port: " << e.what() << std::endl;
				_port = 80; // Default port
			}
		}
		else
		{
			_port = 80;
		}
	}
	else
	{
		_port = 80;
	}
}

void HttpRequest::getHost()
{
	if (_headers.find("host") != _headers.end())
	{
		std::string host_header = _headers["host"];
		size_t colon_pos = host_header.find(':');
		if (colon_pos != std::string::npos)
		{
			_host = host_header.substr(0, colon_pos);
		}
		else
		{
			_host = host_header;
		}
	}
	else
	{
		_host = "";
	}
}

void HttpRequest::parseHeaders(const std::string &header_lines)
{
	size_t pos = 0;
	while (_valid_request)
	{
		size_t line_end = header_lines.find("\r\n", pos);
		if (line_end == std::string::npos || line_end == pos)
			break;

		std::string header_line = header_lines.substr(pos, line_end - pos);

		if (header_line.find(" :") != std::string::npos ||
			header_line.find("\t:") != std::string::npos)
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: invalid header format." << std::endl; // For debugging
			return;
		}

		size_t colon_pos = header_line.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string header_name = header_line.substr(0, colon_pos);
			std::string header_value = header_line.substr(colon_pos + 1);

			if (!isValidHeader(header_name, header_value))
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: invalid header name or value." << std::endl; // For debugging
				return;
			}
			toLowerCase(header_name);
			std::string trimmed_value = trim(header_value);
			_headers[header_name] = trimmed_value;
			//std::cout << "Header parsed: " << header_name << " => " << trimmed_value << std::endl; // For debugging
		}
		pos = line_end + 2;
	}
	getPort();
	getHost();
	return;
}

void HttpRequest::parseChunkedBody(const std::string &body)
{
	try
	{
		bool parse_body_complete = false;
		size_t chunk_length = 0;
		size_t pos = 0;
		while (!parse_body_complete)
		{
			if (!isxdigit(body[pos]))
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: invalid chunk size." << std::endl; // For debugging
				return;
			}
			size_t i = 1;
			while (isxdigit(body[pos + i]))
				i++;
			chunk_length = strtol(body.substr(pos, i).c_str(), NULL, 16);
			pos += i;

			while (body[pos] != '\r')
				pos++;
			if (body.substr(pos, 2) != "\r\n")
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: missing CRLF after chunk size." << std::endl; // For debugging
				return;
			}

			pos += 2;

			if (chunk_length == 0)
			{
				parse_body_complete = true;
				break;
			}

			std::string chunk_data = body.substr(pos, chunk_length);
			_body += chunk_data;
			pos += chunk_length;

			if (body.substr(pos, 2) != "\r\n")
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: missing CRLF after chunk." << std::endl; // For debugging
				return;
			}
			pos += 2;
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
}

void HttpRequest::parseBody(const std::string &body)
{
	//Check for server client max body size? (From config file)
	//If exceeded, set _valid_request to false and _status_code to 413
	if (_headers.find("transfer-encoding") != _headers.end())
	{
		if (_headers["transfer-encoding"].find_first_of("chunked") != std::string::npos)
		{
			parseChunkedBody(body);
			return;
		} else 
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: unsupported Transfer-Encoding." << std::endl; // For debugging
			return;
		}

	}  
	else if (_headers.find("Content-Length") != _headers.end())
	{
		try 
		{
			size_t content_length = static_cast<size_t>(strtol(_headers["Content-Length"].c_str(), NULL, 10));
			
			if (body.size() > content_length)
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: body too large." << std::endl; // For debugging
				return;
			}
			else if (body.size() < content_length)
			{
				_valid_request = false;
				_status_code = 400;
				std::cout << "Invalid request: body too small." << std::endl; // For debugging
				return;
			}
			_body = body;
		}
		catch (const std::exception&)
		{
			_valid_request = false;
			_status_code = 400;
			std::cout << "Invalid request: exception parsing Content-Length." << std::endl; // For debugging
			return;
		}
	}
	else if (_headers.find("Content-Length") == _headers.end())
	{
		_valid_request = false;
		_status_code = 411;
		std::cout << "Invalid request: missing Content-Length." << std::endl; // For debugging
		return;
	}
}

void HttpRequest::parseRequest(const std::string &rawRequest)
{
	HttpRequest request;
	size_t pos = 0;
	size_t line_end = rawRequest.find("\r\n");

	if (line_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: no CRLF found in request line." << std::endl; //For debugging
		return;
	}

	std::string requestLine = rawRequest.substr(0, line_end);
	pos = line_end + 2;

	parseRequestLine(requestLine);
	if (!_valid_request)
		return;
	if (!isValidVersion(_version))
		return;
	size_t header_end = rawRequest.find("\r\n\r\n", pos);

	if (header_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		std::cout << "Invalid request: no CRLFCRLF found." << std::endl; //For debugging
		return;
	}
	if (_valid_request == false)
		return;
	std::string header_lines = rawRequest.substr(pos, header_end - pos + 2);
	pos = header_end + 4;
	
	parseHeaders(header_lines);

	if (pos < rawRequest.size() && _valid_request) // Check headers before parsing body
	{
		std::string body = rawRequest.substr(pos);
		parseBody(body);
	}

	return;
}
bool HttpRequest::isValidRequest() const
{
	return _valid_request;
}