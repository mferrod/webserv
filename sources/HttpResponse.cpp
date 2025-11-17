#include "../includes/HttpResponse.hpp"

HttpResponse::HttpResponse()
{
	_status_code = 0;
	_reason = "";
	_headers.clear();
	_body = "";
}

HttpResponse::~HttpResponse() {}

HttpResponse::HttpResponse(const HttpResponse &other)
{
	_status_code = other._status_code;
	_reason = other._reason;
	_headers = other._headers;
	_body = other._body;
	_request = other._request;
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other)
{
	if (this != &other)
	{
		_status_code = other._status_code;
		_reason = other._reason;
		_headers = other._headers;
		_body = other._body;
		_request = other._request;
	}
	return *this;
}

HttpResponse::HttpResponse(const HttpRequest &req)
{
	_status_code = req.getStatusCode();
	_reason = "";
	_headers.clear();
	_body = "";
	_request = req;
}

/* void HttpResponse::handleGet()
{
	
} */

void HttpResponse::handleRequest()
{
	if (_request.isValidRequest())
	{
		if (_request.getMethod() == "GET")
		{
			handleGet();
		}
		else if (_request.getMethod() == "POST")
		{
			handlePost();
		}
		else if (_request.getMethod() == "DELETE")
		{
			handleDelete();
		}
		/* else //Necessary?
		{
			makeErrorResponse();
		} */
	}
	else
	{
		makeErrorResponse();
	}
}

std::string HttpResponse::getReason()
{
	switch (_status_code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 204: return "No Content";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 400: return "Bad Request";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 417: return "Expectation Failed";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		case 507: return "Insufficient Storage";
		default: return "Internal Server Error";
	}
}

void HttpResponse::makeErrorResponse()
{
	_status_code = _request.getStatusCode();
	_reason = getReason();
	_body = "<html><body><h1>" + std::to_string(_status_code) + " " + _reason + "</h1></body></html>";
	_headers["Content-Type"] = "text/html";
	_headers["Content-Length"] = std::to_string(_body.size());
	return;
}

std::string HttpResponse::buildResponse()
{
	std::string response;

	response += "HTTP/1.1 " + std::to_string(_status_code) + " " + _reason + "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response += it->first + ": " + it->second + "\r\n";
	}

	response += "\r\n";
	response += _body;

	return response;
}