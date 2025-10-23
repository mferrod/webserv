#include "../includes/HttpRequest.hpp"

HttpRequest::HttpRequest()
{
	_method = "";
	_path = "";
	_version = "";
	_headers.clear();
	_body = "";
	_status_code = 0;
	_valid_request = true;
}

HttpRequest::~HttpRequest() {}

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

bool HttpRequest::isAllowedCharPath(u_int8_t c) const
{
	if ((ch >= '#' && ch <= ';') || (ch >= '?' && ch <= '[') || (ch >= 'a' && ch <= 'z') ||
		ch == '!' || ch == '=' || ch == ']' || ch == '_' || ch == '~')
		return (true);
	return (false);
}

bool HttpRequest::isValidPath(const std::string &path) const
{
	if (path.empty())
		return false;
	
	if (path[0] != '/')
		return false;
	
	if (path.length() > 8192)
		return false;
	
	for (size_t i = 1; i < path.length(); i++)
	{
		u_int8_t c = path[i];
		
		if (!isAllowedCharPath(c))
		{
			_status_code = 400;
			return false;
		}
	}
	
	if (path.find("../") != std::string::npos || 
		path.find("/..") != std::string::npos ||
		path == ".." || 
		path.find("/../") != std::string::npos)
		return false;
	
	if (path.find("//") != std::string::npos)
		return false;
	
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
			u_int8_t c = static_cast<u_int8_t>(std::stoi(hex, nullptr, 16));

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

void HttpRequest::parseRequestLine(const std::string &line)
{
	size_t method_end = line.find(' ');
	if (method_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		return;
	}
	_method = line.substr(0, method_end);

	if (!isValidMethod(_method))
	{
		_valid_request = false;
		_status_code = 400;  // Invalid method
		return;
	}

	if (!isImplementedMethod(_method))
	{
		_valid_request = false;
		_status_code = 501;  // Not implemented method
		return;
	}
	
	size_t path_end = line.find(' ', method_end + 1);
	if (path_end == std::string::npos)
	{
		_valid_request = false;
		_status_code = 400;
		return;
	}

	std::string full_path = line.substr(method_end + 1, path_end - method_end - 1);

	full_path = normalizePath(full_path);

	if (!isValidPath(full_path))
	{
		_valid_request = false;
		_status_code = 400;
		return;
	}

	if (full_path.find('?') != std::string::npos)
	{
		_path = full_path.substr(0, full_path.find('?'));
		_path_query = full_path.substr(full_path.find('?') + 1);
	}
	else if (full_path.find('#') != std::string::npos)
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
		return;
	}
	_version = line.substr(path_end + 1);
	return;
}
void HttpRequest::parseHeaders(const std::string &header_lines)
{
	size_t pos = 0;
	while (true)
	{
		size_t line_end = header_lines.find("\r\n", pos);
		if (line_end == std::string::npos || line_end == pos)
			break;

		std::string header_line = header_lines.substr(pos, line_end - pos);

		if (headerLine.find(" :") != std::string::npos ||
			headerLine.find("\t:") != std::string::npos)
		{
			_valid_request = false;
			_status_code = 400;
			return;
		}

		size_t colon_pos = header_line.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string header_name = header_line.substr(0, colon_pos);
			std::string header_value = header_line.substr(colon_pos + 1);
			while (!header_value.empty() && (header_value[0] == ' ' || header_value[0] == '\t'))
			{
				header_value.erase(0, 1);
			}
			_headers[header_name] = header_value;
		}
		pos = line_end + 2;
	}
	return;
}

void HttpRequest::parseBody(const std::string &body)
{
	if (_headers.find("Content-Length") != _headers.end())
	{
		try 
		{
			if (std::count(_headers.begin(), _headers.end(), "Content-Length") > 1)
			{
				_valid_request = false;
				_status_code = 400;
				return;
			}

			size_t content_length = static_cast<size_t>(std::stoi(_headers["Content-Length"]));
			
			if (body.size() > content_length)
			{
				_valid_request = false;
				_status_code = 400;
				return;
			}
			else if (body.size() < content_length)
			{
				_valid_request = false;
				_status_code = 400;
				return;
			}
			
			_body = body;
		}
		catch (const std::exception&)
		{
			_valid_request = false;
			_status_code = 400;
			return;
		}
	}
	else
		_body = body;
	return;
}

void HttpRequest::parseRequest(const std::string &rawRequest)
{
	HttpRequest request;
	size_t pos = 0;
	size_t line_end = rawRequest.find("\r\n");

	if (line_end == std::string::npos)
		_valid_request = false;
		_status_code = 400;
		return;

	std::string requestLine = rawRequest.substr(0, line_end);
	pos = line_end + 2;

	parseRequestLine(requestLine);
	
	size_t header_end = rawRequest.find("\r\n\r\n", pos);

	if (header_end == std::string::npos || _valid_request == false)
	{
		_valid_request = false;
		_status_code = 400;
		return;
	}

	std::string header_lines = rawRequest.substr(pos, header_end - pos);
	pos = header_end + 4;
	
	parseHeaders(header_lines);

	if (pos < rawRequest.size() && _valid_request == true)
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