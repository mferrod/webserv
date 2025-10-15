#pragma once
#include <string>
#include <map>

struct HttpRequest
{
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
};

HttpRequest parseRequest(const std::string &rawRequest);

// For debugging
void printRequest() const
{
	std::cout << "Method: " << _request.method << std::endl;
	std::cout << "Path: " << _request.path << std::endl;
	std::cout << "Version: " << _request.version << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin(); it != _request.headers.end(); ++it)
	{
		std::cout << it->first << ": " << it->second << std::endl;
	}
	std::cout << "Body: " << _request.body << std::endl;
}