#include "httpUtils.hpp"

std::string trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \t");
	size_t last = str.find_last_not_of(" \t");
	return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}

bool caseInsensitiveCompare(const std::string &a, const std::string &b) // comparación case-insensitive
{
	if (a.size() != b.size())
		return false;
	for (size_t i = 0; i < a.size(); i++)
	{
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	}
	return true;
}

bool isToken(const std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if (!isalnum(c) && c != '!' && c != '#' && c != '$' && c != '%' &&
			c != '&' && c != '\'' && c != '*' && c != '+' && c != '-' &&
			c != '.' && c != '^' && c != '_' && c != '`' && c != '|' && c != '~')
		{
			return false;
		}
	}
	return true;
}