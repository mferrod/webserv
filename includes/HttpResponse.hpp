#pragma once
#include <string>
#include <map>
#include "HttpRequest.hpp"

class HttpResponse
{
	private:
		
		int 								_status_code;
		std::string 						_reason;
		std::map<std::string, std::string>	_headers;
		std::string 						_body;
		HttpRequest 						&_request;
		//ServerConfig 						_fake_server_config; // Not implemented yet.

	public:
	
		HttpResponse();
		HttpResponse(const HttpRequest &req);
		~HttpResponse();
		
		void 			handleRequest();
		void 			handleGet();
		void 			handlePost();
		void 			handleDelete();
		void 			makeErrorResponse();
		std::string 	getReason();
		std::string 	buildResponse();
};