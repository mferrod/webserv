#pragma once
#include <string>
#include <map>
#include <cstdint>
#include "httpUtils.hpp"

class HttpRequest
{
	public:
		HttpRequest();
		~HttpRequest();

	private:
		std::string 						_method;
		std::string 						_path;
		std::string 						_path_query;
		std::string 						_path_fragment;
		std::string 						_version;
		std::map<std::string, std::string> 	_headers;
		std::string 						_body;
		int 								_status_code;
		bool 								_valid_request;

		void parseRequest(const std::string &rawRequest);

		void parseRequestLine(const std::string &line);
		void parseHeaders(const std::string &header_lines);
		void parseBody(const std::string &body);

		bool isValidMethod(const std::string &method) const;
		bool isImplementedMethod(const std::string &method) const;
		bool isValidPath(const std::string &path) const;
		std::string normalizePath(const std::string &path) const;
		bool isAllowedCharPath(u_int8_t c) const;
		bool isValidVersion(const std::string &version) const;
		
		bool isValidHeaderValue(const std::string &value) const;
		bool isValidHeader(const std::string &name, const std::string &value) const;
		
		bool isValidRequest() const;
};



