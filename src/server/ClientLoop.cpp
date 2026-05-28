#include "Server.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>

void	Server::clientLoop()
{
	int n;
	while (true)
	{
		// im epolling it up rn
		n = epoll_wait(epoll_fd, events, 64, -1);
		if (n < 0)
			throw std::runtime_error("epoll_wait() error");
		for (int i = 0; i < n; i++)
		{
			Logger::printLog("event fd: {} socket_fd: {}", events[i].data.fd, socket_fd);
			if (events[i].data.fd == socket_fd)
				acceptClient(events[i].data.fd);
			else
				handleClient(events[i].data.fd);
		}
	}
}

std::string getContentType(const std::string &path)
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
	return ("application/octet-stream");
}

std::string buildHTTPResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type)
{
	std::string response;

	response += "HTTP/1.1" + code + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";
	response += "Content-Length: " + std::to_string(size) + "\r\n\r\n";
	response += body;
	return (response);
}

Parser::LocationConfig *Server::matchLocation(const std::string &uri)
{
	Parser::LocationConfig *match = nullptr;
	size_t longest = 0;
	for (Parser::LocationConfig &loc : server_conf[0].locations)
	{
		if (uri.find(loc.path) == 0 && loc.path.size() > longest)
		{
			longest = loc.path.size();
			match = &loc;
		}
	}
	return (match);
}

std::string get_response_code(int code)
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

void Server::sendError(int fd, int error_code)
{
	std::string filepath = "." + server_conf[0].error_pages[error_code];
	std::ifstream fs(filepath, std::ios::binary);
	if (!fs)
		Logger::printLog("Failed to open file {}", filepath);
	std::stringstream ss_buffer;
	ss_buffer << fs.rdbuf();
	std::string str = ss_buffer.str();
	std::string response = buildHTTPResponse(str.size(), str, get_response_code(error_code), getContentType(filepath));
	Logger::printLog("response: {}", response);
	send(fd, response.c_str(), response.size(), 0);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	fs.close();
	close(fd);
}

void Server::handleGet(int fd, std::string uri, Parser::LocationConfig *loc) // is GET allowed in this location?
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
	std::string response = buildHTTPResponse(str.size(), str, get_response_code(200), getContentType(filepath));
	Logger::printLog("response: {}", response);
	ssize_t read_bytes = 0;
	while (true)
	{
		read_bytes = send(fd, response.c_str(), response.size(), 0);
		if (read_bytes < 0)
			break;
		if (read_bytes == 0)
			break;
		if (read_bytes >= (ssize_t)response.size())
			break;
		response = response.substr(read_bytes, response.size() - read_bytes);
	}
}

void handlePost(int fd, std::string uri, Parser::LocationConfig *loc) // is POST allowed in this location?
{
	(void)fd;
	(void)uri;
	(void)loc;
}

void handleDelete(int fd, std::string uri, Parser::LocationConfig *loc) // is DELETE alloewd in this location?
{
	(void)fd;
	(void)uri;
	(void)loc;
}


void Server::handleClient(int fd)
{
	char	buf[4096];
	ssize_t bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0)
	{
		if (bytes < 0)
			perror("recv() error");
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return ;
	}

	Logger::printLog("received {} bytes from request {}", bytes, buf);

	std::string data(buf);
	std::string method = data.substr(0, data.find(' '));
	std::string uri = data.substr(data.find(' ') + 1, data.find(' ', data.find(' ') + 1) - (data.find(' ') + 1));

	Parser::LocationConfig *loc = matchLocation(uri);
	if (!loc)
	{
		Logger::printLog("404 Not Found!");
		sendError(fd, 404);
		return ;
	}
	Logger::printLog("method: {} uri: {} path: {}", method, uri, loc->root + loc->path + loc->index);
	
	if (std::find(loc->methods.begin(), loc->methods.end(), method) == loc->methods.end())
	{
		Logger::printLog("404 Not Found!");
		sendError(fd, 405);
		return ;
	}

	if (method == "GET")
		handleGet(fd, uri, loc);
	else if (method == "POST")
		handlePost(fd, uri, loc);
	else if (method == "DELETE")
		handleDelete(fd, uri, loc);

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

void	Server::acceptClient(int fd)
{
	sockaddr_in	client_addr;
	socklen_t	addr_len = sizeof(client_addr);
	int			client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);

	if (client_fd < 0)
		return ;
	try {
		registerToEpoll(client_fd);
	} catch (...) {
		close(client_fd);
		Logger::printLog("closed fd: {}", client_fd);
		throw ;
	}
	Logger::printLog("New client at fd: {} and ip: {}", client_fd, inet_ntoa(client_addr.sin_addr));
}