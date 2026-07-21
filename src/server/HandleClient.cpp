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
#include <algorithm> //couldn't compile at home without

static	std::string matchExtension(const std::string &uri)
{
	std::string	ret{};
	size_t		extension_loc = uri.rfind('.');

	if (extension_loc != ret.npos)
		ret = uri.substr(extension_loc);
	return (ret);
}

Parser::LocationConfig *Server::matchLocation(const std::string &uri, const int fd)
{
	Parser::LocationConfig *match = nullptr;
	size_t longest = 0;

	for (Parser::LocationConfig &loc : server_conf[client_to_conf[fd]].locations)
	{
		if (uri.find(loc.path) == 0 && loc.path.size() > longest)
		{
			longest = loc.path.size();
			match = &loc;
		}
	}
	return (match);
}

bool	Server::fetchRequest(int fd)
{
	char	buf[4096];

	bzero(buf, 4096);
	ssize_t bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0)
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		client_buffers.erase(fd);
		return (false);
	}
	client_buffers[fd].append(buf, bytes);

	size_t headers_end = client_buffers[fd].find("\r\n\r\n");
	if (headers_end == client_buffers[fd].npos)
		return (false);
	headers_end += 4;

	size_t content_len = 0;
	size_t pos = client_buffers[fd].find("Content-Length:");
	if (pos != client_buffers[fd].npos)
		content_len = std::stoul(client_buffers[fd].substr(pos + 16, client_buffers[fd].find("\r\n", pos + 16) - (pos + 16)));
	Logger::printLog("{} {} {}", client_buffers[fd].size(), headers_end, content_len);
	if (client_buffers[fd].size() - headers_end < content_len)
		return (false);
	Logger::printLog("received {} bytes from request {}", content_len, client_buffers[fd]);
	return (true);
}

void	Server::handleClient(int fd)
{
	if (!fetchRequest(fd))
		return ;
	HTTP::Request req = HTTP::parse(client_buffers[fd]);
	client_buffers.erase(fd);

	Parser::LocationConfig *loc = matchLocation(req.uri, fd);
	if (!loc)
	{
		Logger::printLog("404 Not Found!");
		sendError(fd, 404);
		return ;
	}
	Logger::printLog("method: {} uri: {} path: {}", req.method, req.uri, loc->root + loc->path + loc->index);
	
	if (loc->return_code != 0)
	{
		handleRedir(fd, req.uri, loc, req);
		return ;
	}

	if (std::find(loc->methods.begin(), loc->methods.end(), req.method) == loc->methods.end())
	{
		Logger::printLog("405 Method Not Allowed!");
		sendError(fd, 405);
		return ;
	}

	std::map<std::string, std::string>::iterator it = loc->cgi.find(matchExtension(req.uri));
	if (it != loc->cgi.end())
	{
		// std::string script = req.uri.substr(loc->path.size());
		// std::string filepath = "." + loc->root + script;
		handleCGI(fd, req, loc, it->second);
		return ;
	}
	else if (req.method == "GET")
		handleGet(fd, req.uri, loc);
	else if (req.method == "POST")
		handlePost(fd, req.uri, loc, req);
	else if (req.method == "DELETE")
		handleDelete(fd, req.uri, loc);
}