#include "Server.hpp"
#include "HTTP.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>

std::string	Server::getContentType(const std::string &path)
{
	if (path.ends_with(".html"))
		return ("text/html");
	if (path.ends_with(".png"))
		return ("image/png");
	if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
		return ("image/jpeg");
	if (path.ends_with(".css"))
		return ("text/css");
	if (path.ends_with(".js"))
		return ("application/javascript");
	if (path.ends_with(".gif"))
		return ("image/gif");
	return ("application/octet-stream");
}

void	Server::handleDelete(int fd, std::string uri, Parser::LocationConfig *loc)
{
	std::string root = loc->root.empty() ? loc->upload_store : loc->root;
	std::string filepath = "." + root + uri;

	if (access(filepath.c_str(), F_OK) == -1)
	{
		sendError(fd, 404);
		return ;
	}

	if (access(filepath.c_str(), W_OK) == -1)
	{
		sendError(fd, 403);
		return ;
	}

	if (unlink(filepath.c_str()) == -1)
	{
		sendError(fd, 500);
		return ;
	}

	std::string response = HTTP::buildResponse(0, "", HTTP::getResponseCode(200), getContentType(filepath));
	if (send(fd, response.c_str(), response.size(), 0) == -1)
		Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close (fd);
}

void	Server::handleGet(int fd, std::string uri, Parser::LocationConfig *loc)
{
	std::string filepath = "." + loc->root + uri;

	if (!uri.empty() && uri.back() == '/')
		filepath += loc->index;

	std::ifstream fs(filepath, std::ios::binary);
	std::string str;
	if (!fs)
	{
		sendError(fd, 404);
		return ;
	}
	std::stringstream ss_buffer;
	ss_buffer << fs.rdbuf();
	str = ss_buffer.str();
	std::string response = HTTP::buildResponse(str.size(), str, HTTP::getResponseCode(200), getContentType(filepath));
	Logger::printLog("response: {}", response);
	// ssize_t read_bytes = 0;
	sendResponse(fd, response);
	// if (read_bytes < 0)
	// {
	// 	Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
	// 	break;
	// }
	// if (read_bytes == 0)
	// 	break;
	// if (read_bytes >= (ssize_t)response.size())
	// 	break;
	// response = response.substr(read_bytes, response.size() - read_bytes);
}

static bool isDirectory(const std::string &path)
{
	struct stat s;
	if (stat(path.c_str(), &s) == 0)
		return (S_ISDIR(s.st_mode));
	return (false);
}

// add file extension?
void Server::handlePost(int fd, std::string &uri, Parser::LocationConfig *loc, HTTP::Request &req)
{
	std::string filepath = "." + loc->root + uri;

	if (isDirectory(filepath) && !req.pd.empty)
		filepath += "/" + req.pd.file_name;
	if (!uri.empty() && uri.back() == '/')
		filepath += loc->index;
	if (isDirectory(filepath))
		filepath += static_cast<std::string>("/upload_") + Logger::getTime("%m%d%H%M%S");

	Logger::printLog("filepath: {} root: {} uri: {}", filepath, loc->root, uri);

	std::ofstream of(filepath, std::ios::out | std::ios::binary | std::ios::trunc);

	if (!of)
	{
		std::string body = "<html><body>500 Internal Server Error</body></html>";
		std::string response = HTTP::buildResponse(body.size(), body, HTTP::getResponseCode(500), getContentType(".html"));
		if (send(fd, response.c_str(), response.size(), 0) == -1)
			Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
		return ;
	}

	of << req.body;
	of.close();

	std::string body = "<html><body>OK</body></html>";
	std::string response = HTTP::buildResponse(body.size(), body, HTTP::getResponseCode(200), getContentType(".html"));
	sendResponse(fd, response);
	// if (send(fd, response.c_str(), response.size(), 0) == -1)
		// Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
}

void	Server::handleRedir(int fd, std::string &uri, Parser::LocationConfig *loc, HTTP::Request &req)
{
	(void)uri; //uri not needed?
	std::string	target = loc->return_url;
	std::string	code = std::to_string(loc->return_code);
	if (!req.query.empty() && target.find('?') == std::string::npos)
		target += "?" + req.query;
	std::string body = "<html><body>Redirecting to " + target + "</body></html>";
	std::string response = HTTP::buildResponse(body.size(), body, code, "text/html", target); // overloads r pog
	sendResponse(fd, response);
	// if (send(fd, response.c_str(), response.size(), 0) == -1)
			// Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
	// absolute vs relative paths? Any input?
}