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

std::string HTTP::getQuery(std::string &uri)
{
	std::string	query{};
	size_t		qloc;

	qloc = uri.find('?');
	if (qloc != uri.npos)
	{
		query = uri.substr(qloc + 1);
		uri = uri.substr(0, qloc);
	}
	return (query);
}

HTTP::Request HTTP::parse(const std::string &data)
{
	Request	req;
	std::istringstream iss(data);
	std::string ln;

	iss >> req.method >> req.uri >> req.version;
	req.query = getQuery(req.uri);
	iss.ignore();
	while (std::getline(iss, ln))
	{
		if (ln == "\r")
			break;
		size_t breakpoint = ln.find(':');
		if (breakpoint != ln.npos)
		{
			req.headers[ln.substr(0, breakpoint)] = ln.substr(breakpoint + 2);
			Logger::printLog("header: |{}| |{}|", ln.substr(0, breakpoint), ln.substr(breakpoint + 2));
		}
	}
	if (req.headers["Content-Type"].find("multipart/form-data") != req.headers["Content-Type"].npos
		&& !req.headers["Content-Length"].empty())
		req.pd = getPostData(iss, req.body, std::stoul(req.headers["Content-Length"]));
	else if (!req.headers["Content-Length"].empty())
		req.body = data.substr(data.find("\r\n\r\n") + 4, std::stoul(req.headers["Content-Length"]));
	Logger::printLog("bodybag: {}", req.body);
	return (req);
}

HTTP::postData	HTTP::getPostData(std::istringstream &iss, std::string &body, size_t body_max)
{
	postData pd;
	std::string ln;

	iss.ignore(2);
	while (std::getline(iss, ln))
	{
		if (ln == "\r")
			break;
		size_t pos = ln.find("filename=");
		if (pos != ln.npos)
		{
			pos += 10;
			pd.file_name = ln.substr(pos, ln.find('"', pos) - (pos));
		}
		pos = ln.find("name=");
		if (pos != ln.npos)
		{
			pos += 5;
			pd.type_name = ln.substr(pos, ln.find('"', pos) - (pos));
		}
	}
	while (std::getline(iss, ln) && body_max > 0)
	{
		if (ln == "\r")
			break;
		body += ln.substr(0, body_max);
		if (ln.size() > body_max)
			body_max = 0;
		else
			body_max -= ln.size(); 
	}
	pd.empty = false;
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