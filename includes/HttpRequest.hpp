#pragma once
#include <string>
#include <map>
#include <stdint.h>
#include <exception>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <iostream>
#include "httpUtils.hpp"
#include "ServerConfig.hpp"
#include "Location.hpp"

class HttpRequest
{
	public:
		HttpRequest();
		~HttpRequest();
		HttpRequest(const HttpRequest &other);
		HttpRequest &operator=(const HttpRequest &other);

	private:
		std::string 						_method;
		std::string 						_path;
		std::string 						_path_query;
		std::string 						_path_fragment;
		std::string 						_path_dir;
		std::string 						_path_file;
		std::string 						_path_info;
		std::string 						_version;
		std::map<std::string, std::string> 	_headers;
		std::string 						_body;
		bool 								_valid_request;
		int 								_status_code;
		int									_port;
		std::string							_host;
		Location							_target_location;

	public:
		
		void parseRequest(const std::string &rawRequest);

		void parseRequestLine(const std::string &line);
		void parseHeaders(const std::string &header_lines);
		void parseBody(const std::string &body);
		void parseChunkedBody(const std::string &body);

		bool isValidMethod(const std::string &method) const;
		bool isImplementedMethod(const std::string &method) const;
		bool isValidPath(const std::string &path);
		std::string normalizePath(const std::string &path) const;
		bool isAllowedCharPath(unsigned char c) const;
		bool isValidVersion(const std::string &version);
		
		bool isValidHeaderValue(const std::string &value) const;
		bool isValidHeader(const std::string &name, const std::string &value) const;

		void getPortHeader();
		void getHostHeader();

		void handleTargetLocation();
		void handleGet();
		void handlePost();
		void handleDelete();

		void checkBodySize(const ServerConfig &serverConfig);
		
		bool isValidRequest() const;
		//void setStatusCode(int code) { _status_code = code; }

		void setTargetLocation(const Location &location) { _target_location = location; }
		void setValidRequest(bool valid) { _valid_request = valid; }
		void setPathFile(const std::string &file) { _path_file = file; }

		int									getStatusCode() const { return _status_code; }
		std::string							&getMethod() { return _method; }
		std::string							&getPath() { return _path; }
		std::string							&getVersion() { return _version; }
		std::map<std::string, std::string>	&getHeaders() { return _headers; }
		std::string							&getBody() { return _body; }
		Location							getTargetLocation() const { return _target_location; }
		std::string							getFile() const { return _path_file; }

		// For debugging
		void printRequest() const {
			if (!_valid_request) {
				std::cout << "Invalid HTTP Request. Status Code: " << _status_code << std::endl;
				return;
			} else {
			std::cout << "Method: " << _method << std::endl;
			std::cout << "Path: " << _path << std::endl;
			std::cout << "Version: " << _version << std::endl;
			for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
				std::cout << it->first << ": " << it->second << std::endl;
			}
			if (!_body.empty())
				std::cout << "Body: " << _body << std::endl;
			}
		}
};