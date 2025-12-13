#pragma once
#include <string>
#include <map>
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include "ServerConfig.hpp" //?? Not implemented yet.
#include "HttpRequest.hpp"
#include "ServerSocket.hpp"
#include "CGI.hpp"

class HttpResponse
{
	private:
		
		int 								_status_code;
		std::string 						_reason;
		std::map<std::string, std::string>	_headers;
		std::string 						_body;
		HttpRequest 						_request;

	public:
	
		HttpResponse();
		HttpResponse(const HttpResponse &other);
		HttpResponse(const HttpRequest &req);
		HttpResponse &operator=(const HttpResponse &other);
		~HttpResponse();
		
		void 			handleRequest(std::vector<ServerSocket> &servers);
		void 			handleGet(const ServerConfig &server_config);
		void 			handlePost();
		void 			handleDelete(const ServerConfig &server_config);
		void 			makeErrorResponse();
		std::string 	getReason();
		std::string 	buildResponse();
		void			handleTargetLocation(const ServerConfig &server_config);
		void			isAllowedMethod();
		void			checkClientMaxBodySize();
		void			redirection(std::string url);
		void			readFile(const std::string &file_path);
		void			makeAutoindex();
		void			handleCGI();
};