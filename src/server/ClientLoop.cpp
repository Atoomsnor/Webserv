#include "Server.hpp"
#include "HTTP.hpp"
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
			if (events[i].data.fd == socket_fd)
				acceptClient(events[i].data.fd);
			else
				handleClient(events[i].data.fd);
		}
	}
}

std::string Server::getContentType(const std::string &path)
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

void Server::sendError(int fd, int error_code)
{
	std::string filepath = "." + server_conf[0].error_pages[error_code];
	std::ifstream fs(filepath, std::ios::binary);
	if (!fs)
		Logger::printLog("Failed to open file {}", filepath); // TODO return
	std::stringstream ss_buffer;
	ss_buffer << fs.rdbuf();
	std::string str = ss_buffer.str();
	std::string response = HTTP::buildResponse(str.size(), str, HTTP::getResponseCode(error_code), getContentType(filepath));
	Logger::printLog("response: {}", response);
	send(fd, response.c_str(), response.size(), 0);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	fs.close();
	close(fd);
}

void Server::handleGet(int fd, std::string uri, Parser::LocationConfig *loc)
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
	ssize_t read_bytes = 0;
	while (true)
	{
		read_bytes = send(fd, response.c_str(), response.size(), 0);
		if (read_bytes < 0)
			break; // TODO check whats needs to happen in case?
		if (read_bytes == 0)
			break; // TODO check whats needs to happen in case?
		if (read_bytes >= (ssize_t)response.size())
			break;
		response = response.substr(read_bytes, response.size() - read_bytes);
	}
}

void Server::handlePost(int fd, std::string &uri, Parser::LocationConfig *loc, HTTP::Request &req)
{
	std::string filepath = "." + loc->root + uri;

	Logger::printLog("filepath: {} root: {} uri: {}", filepath, loc->root, uri);
	if (!uri.empty() && uri.back() == '/')
		filepath += loc->index;

	// std::ifstream fs(filepath);
	// if (fs)
	// {
	//  filepath += " 1";
	// }

	std::ofstream of(filepath, std::ios::binary);

	of << req.body;
	of.close();

	std::string body = "<html><body>OK</body></html>";
	std::string response = HTTP::buildResponse(body.size(), body, HTTP::getResponseCode(200), getContentType(".html"));
	send(fd, response.c_str(), response.size(), 0);
}

void Server::handleDelete(int fd, std::string uri, Parser::LocationConfig *loc)
{
	std::string root = loc->root.empty() ? loc->upload_store : loc->root;
	std::string filepath = "." + loc->root + uri;

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
	send(fd, response.c_str(), response.size(), 0);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close (fd);
}


void Server::handleClient(int fd)
{
	char	buf[4096];

	bzero(buf, 4096);
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

	HTTP::Request req = HTTP::parse(std::string(buf, bytes));
	
	Logger::printLog("method: '{}' uri: '{}'", req.method, req.uri);
	Parser::LocationConfig *loc = matchLocation(req.uri);
	if (!loc)
	{
		Logger::printLog("404 Not Found!");
		sendError(fd, 404);
		return ;
	}
	Logger::printLog("method: {} uri: {} path: {}", req.method, req.uri, loc->root + loc->path + loc->index);
	
	if (std::find(loc->methods.begin(), loc->methods.end(), req.method) == loc->methods.end())
	{
		Logger::printLog("404 Not Found!");
		sendError(fd, 405);
		return ;
	}

	if (req.method == "GET")
		handleGet(fd, req.uri, loc);
	else if (req.method == "POST")
		handlePost(fd, req.uri, loc, req);
	else if (req.method == "DELETE")
		handleDelete(fd, req.uri, loc);

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