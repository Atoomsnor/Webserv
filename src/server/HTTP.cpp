#include "HTTP.hpp"
#include "Logger.hpp"
#include <sstream>

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
#include <iostream>
HTTP::Request HTTP::parse(const std::string &data)
{
	Request	req;
	std::istringstream iss(data);
	std::string ln;

	iss >> req.method >> req.uri >> req.version;
	while (std::getline(iss, ln))
	{
		if (ln == "\r\n")
			break;
		size_t breakpoint = ln.find(':');
		if (breakpoint != ln.npos)
		{
			req.headers[ln.substr(0, breakpoint)] = ln.substr(breakpoint + 1);
			Logger::printLog("header: |{}| |{}|", ln.substr(0, breakpoint), ln.substr(breakpoint + 1));
		}
		
	}
	if (!req.headers["Content-Length"].empty())
		req.body = data.substr(data.find("\r\n\r\n") + 4, std::stoul(req.headers["Content-Length"]));
	if (req.headers["Content-Type"].find("multipart/form-data") != req.headers["Content-Type"].npos)
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

std::string	HTTP::buildResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type)
{
	std::string response;

	response += "HTTP/1.1" + code + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + std::to_string(size) + "\r\n\r\n";
	response += body;
	return (response);
} 