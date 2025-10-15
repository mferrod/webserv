#include "Request.hpp"

HttpRequest parseRequest(const std::string &rawRequest)
{
	HttpRequest request;
	size_t pos = 0;
	size_t lineEnd = rawRequest.find("\r\n");

	if (lineEnd == std::string::npos)
		return request;

	std::string requestLine = rawRequest.substr(0, lineEnd);
	pos = lineEnd + 2;

	size_t methodEnd = requestLine.find(' ');
	if (methodEnd == std::string::npos)
		return request;
	request.method = requestLine.substr(0, methodEnd);

	size_t pathEnd = requestLine.find(' ', methodEnd + 1);
	if (pathEnd == std::string::npos)
		return request;
	request.path = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);

	request.version = requestLine.substr(pathEnd + 1);

	while (true)
	{
		lineEnd = rawRequest.find("\r\n", pos);
		if (lineEnd == std::string::npos || lineEnd == pos)
			break;

		std::string headerLine = rawRequest.substr(pos, lineEnd - pos);
		size_t colonPos = headerLine.find(':');
		if (colonPos != std::string::npos)
		{
			std::string headerName = headerLine.substr(0, colonPos);
			std::string headerValue = headerLine.substr(colonPos + 1);
			while (!headerValue.empty() && (headerValue[0] == ' ' || headerValue[0] == '\t'))
			{
				headerValue.erase(0, 1);
			}
			request.headers[headerName] = headerValue;
		}
		pos = lineEnd + 2;
	}

	if (pos < rawRequest.size())
	{
		request.body = rawRequest.substr(pos);
	}

	return request;
}