#include "HTTP.hpp"
#include "Logger.hpp"
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <iostream>

HTTP::Request	HTTP::parse(const std::string &data)
{
	Request	req;
	std::istringstream iss(data, std::ios::binary);
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
			std::string value = ln.substr(breakpoint + 2);
			if (!value.empty() && value.back() == '\r')
				value.pop_back();
			req.headers[ln.substr(0, breakpoint)] = value;
		}
	}
	if (req.headers["Content-Type"].find("multipart/form-data") != req.headers["Content-Type"].npos
		&& !req.headers["Content-Length"].empty())
	{
		req.pd = getPostData(iss);
		req.body = getPDBody(data.substr(data.find("\r\n\r\n") + 4), std::stoul(req.headers["Content-Length"]));
	}
	else if (!req.headers["Content-Length"].empty())
		req.body = data.substr(data.find("\r\n\r\n") + 4, std::stoul(req.headers["Content-Length"]));
	Logger::printLog("bodybag: {}", req.body);
	return (req);
}

std::string	HTTP::getResponseCode(int code)
{
	switch (code) {
		case (301):
			return (" 301 Moved Permanently"); //needed to be handled
		case (302):
			return (" 302 Found"); //not sure if needed to be handled but sould be able to
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
		case(408):
			return (" 408 Request Timeout");
		case(413):
			return (" 413 Payload Too Large");
		case (500):
			return (" 500 Internal Server Error");
		case (502):
			return (" 502 Bad Gateway");
		case (503):
			return (" 503 Service unavailable");
		case (505):
			return (" 505 HTTP Version Not supported");
		default:
			return (" 200 OK");
	}
}

HTTP::postData	HTTP::getPostData(std::istringstream &iss)
{
	postData pd;
	std::string ln;

	iss.ignore(2);
	while (std::getline(iss, ln) && ln != "\r")
	{
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
	pd.empty = false;
	return (pd);
}

std::string	HTTP::getPDBody(const std::string &data, size_t max)
{
	size_t pos1 = data.find("\r\n\r\n");
	if (pos1 == data.npos)
		return (data);
	size_t pos2 = data.find("\r\n--", pos1 + 4);
	if (pos2 == data.npos)
		return (data.substr(pos1 + 4, max));
	if (pos2 > max)
		pos2 = max;
	return (data.substr(pos1 + 4, pos2 - (pos1 + 4)));
}

std::string	HTTP::getQuery(std::string &uri)
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

static std::string buildHref(DIR *dirfd)
{
	dirent *tempdir;
	std::string ret;

	while (true)
	{
		tempdir = readdir(dirfd);
		if (tempdir == NULL)
			break ;
		std::string temp = tempdir->d_name;
		if (temp == "." || temp == "..")
			continue ;
		ret += "\t\t<a href=\"" + temp;
		if (tempdir->d_type == DT_DIR)
			ret += "/";
		ret += "\">" + temp;
		if (tempdir->d_type == DT_DIR)
			ret += "/";
		ret += "</a><br>";
		ret += "\n";
	}
	return (ret);
}

std::string HTTP::buildAutoindex(std::string filepath)
{
	filepath.erase(0, 1);
	// std::cout << "FILEPATH TO OPEN:" << filepath;
	DIR	*dirfd = opendir(filepath.c_str());
	if (dirfd == NULL)
	{
		throw std::runtime_error("failed to open directory"); //don't know if throw is enough
		return NULL;
	}
	std::string autoindex = R"(
<html>
	<head><title>Index of /var</title></head>
	<body>
		<h1>Index of /var</h1>
		<hr>
)";
	autoindex += buildHref(dirfd);
	autoindex += R"(		<hr>
	</body>
</html>)";
	closedir(dirfd);
	return (autoindex);
	// I currently just add eveyrthing in assets, errors leads to a 404, should probably be 403
	// I also show style.css in its entirety.
	// ../skips, however, maybe if the filepath passed is out of bounds, it will still extract, gotta test.
}

std::string	HTTP::buildResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type)
{
	std::string response;

	response += "HTTP/1.1 " + code + "\r\n"; //was missing a space? I don't think code has a space
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + std::to_string(size) + "\r\n";
	response += "\r\n";
	response += body;
	return (response);
}

std::string	HTTP::buildResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type, std::string target)
{
	std::string response;

	response += "HTTP/1.1 " + code + "\r\n"; //was missing a space? I don't think code has a space
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + std::to_string(size) + "\r\n";
	if (!target.empty()) //not really intuitive but for now I Just want to handle 301 and 302
	{
		response += "Location: " + target + "\r\n"; 
	}

	response += "\r\n";
	response += body;
	return (response);
} 