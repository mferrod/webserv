#include "../includes/httpUtils.hpp"

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

void toLowerCase(std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		str[i] = tolower(str[i]);
	}
}

std::string getMimeType(const std::string &file_path)
{
	size_t dot_pos = file_path.find_last_of('.');
	if (dot_pos == std::string::npos)
		return "application/octet-stream"; // Default MIME type

	std::string extension = file_path.substr(dot_pos + 1);
	if (extension == "html" || extension == "htm")
		return "text/html";
	else if (extension == "css")
		return "text/css";
	else if (extension == "js")
		return "application/javascript";
	else if (extension == "png")
		return "image/png";
	else if (extension == "jpg" || extension == "jpeg")
		return "image/jpeg";
	else if (extension == "gif")
		return "image/gif";
	else if (extension == "txt")
		return "text/plain";
	else if (extension == "json")
		return "application/json";
	else if (extension == "xml")
		return "application/xml";
	else
		return "application/octet-stream"; // Default MIME type
}