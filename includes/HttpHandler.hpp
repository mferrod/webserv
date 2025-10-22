#pragma once
#include <string>
#include <map>
#include "Request.hpp"
#include "Response.hpp"

HttpResponse handleGet(const HttpRequest &request);
HttpResponse handlePost(const HttpRequest &request);
HttpResponse handleDelete(const HttpRequest &request);
HttpResponse handleRequest(const HttpRequest &request);

