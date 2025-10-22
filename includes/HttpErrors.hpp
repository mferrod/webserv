#pragma once
#include <string>

std::string defaultErrorPage(int code, const std::string &reason);
std::string getReasonPhrase(int code);