#pragma once
#include <string>

std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, char delim);
bool caseInsensitiveCompare(const std::string &a, const std::string &b);
bool isToken(const std::string &str);