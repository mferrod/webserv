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
		std::string 						_version;
		std::map<std::string, std::string> 	_headers;
		std::string 						_body;
		bool 								_valid_request;
		int 								_status_code;
		
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
		
		bool isValidRequest() const; // Really needed?
		//void setStatusCode(int code) { _status_code = code; }

		int getStatusCode() const { return _status_code; }
		std::string getMethod() const { return _method; }
		std::string getPath() const { return _path; }
		std::string getVersion() const { return _version; }
		std::map<std::string, std::string> getHeaders() const { return _headers; }
		std::string getBody() const { return _body; }

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