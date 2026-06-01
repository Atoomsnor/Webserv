#include "HTTP.hpp"

std::string	HTTP::getResponseCode(int code)
{
	switch (code) {
		case (400):
			return (" 400 Bad Request");
		case (401):
			return (" 401 Unauthorized");
		case (403):
			return (" 403 Forbidden");
		case (404):
			return (" 404 Not Found");
		case (405):
			return (" 405 Method Not Allowed");
		default:
			return (" 200 OK");
	}
}

HTTP::HTTPRequest HTTP::httpParse(const std::string &data)
{
	HTTPRequest	req;

	req.method = data.substr(0, data.find(' '));
	req.uri = data.substr(data.find(' ') + 1, data.find(' ', data.find(' ') + 1) - (data.find(' ') + 1));
	req.body = data.substr(data.find("\r\n\r\n"));
	return (req);
}

HTTP::postData	HTTP::getPostData(const std::string data) // not correct POST method?
{
	postData pd;

	// Logger::printLog("data {} npos {}", data, data.npos);
	size_t pos = data.find("filename=");
	if (pos != data.npos)
	{
		pos += 10;
		pd.file_name = data.substr(pos, data.find('"', pos) - (pos));
	}
	pos = data.find("name=");
	if (pos != data.npos)
	{
		pos += 5;
		pd.type_name = data.substr(pos, data.find('"', pos) - (pos));
	}
	pos = data.find("\r\n\r\n");
	if (pos != data.npos)
	{
		pos += 4;

		size_t end_pos = data.find("\r\n--", pos);
		if (end_pos != data.npos)
			pd.body = data.substr(pos, end_pos - pos);
		else
			pd.body = data.substr(pos);
	}
	return (pd);
}

std::string	HTTP::buildHTTPResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type)
{
	std::string response;

	response += "HTTP/1.1" + code + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + std::to_string(size) + "\r\n\r\n";
	response += body;
	return (response);
} 