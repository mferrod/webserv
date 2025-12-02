#pragma once
#include <string>
#include <map>
#include <sys/stat.h>
#include <fstream>
#include "ServerConfig.hpp" //?? Not implemented yet.
#include "HttpRequest.hpp"

class HttpResponse
{
	private:
		
		int 								_status_code;
		std::string 						_reason;
		std::map<std::string, std::string>	_headers;
		std::string 						_body;
		HttpRequest 						&_request;

	public:
	
		HttpResponse();
		HttpResponse(const HttpResponse &other);
		HttpResponse(const HttpRequest &req);
		HttpResponse &operator=(const HttpResponse &other);
		~HttpResponse();
		
		void 			handleRequest();
		void 			handleGet();
		void 			handlePost();
		void 			handleDelete();
		void 			makeErrorResponse();
		std::string 	getReason();
		std::string 	buildResponse();
		void			handleTargetLocation(const ServerConfig &server_config);
};