#pragma once
#include <string>
#include <map>

struct HttpResponse
{
	int status_code;
	std::string reason;
	std::map<std::string, std::string> headers;
	std::string body;
};

std::string 	buildResponse(const HttpResponse &res);
HttpResponse 	makeErrorResponse(int code, const std::string &message);