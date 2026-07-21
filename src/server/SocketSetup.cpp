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

void	Server::socketSetup()
{
	for (size_t i = 0; i < server_conf.size(); i++)
	{
		int fd = createSocket();
		socket_to_conf[fd] = i;
		bindAndListen(fd, i);
		registerToEpoll(fd, EPOLLIN);
	}
}

int	Server::createSocket()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		throw std::runtime_error("socket() error");
	
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		close(fd);
		throw std::runtime_error("setsockopt() error");
	}
	return (fd);
}

void	Server::bindAndListen(int fd, size_t i)
{
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_conf[i].port);
	Logger::printLog("server_conf.port {}, server_addr.sin_port {}", server_conf[i].port, server_addr.sin_port);
	if (inet_pton(AF_INET, server_conf[i].host.c_str(), &server_addr.sin_addr) != 1)
		throw std::runtime_error("inet_pton() error");
	if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		throw std::runtime_error("bind() error");
		
	if (listen(fd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() error");

	Logger::printLog("sin_port: {} ---- [0].port: {}", server_addr.sin_port, server_conf[i].port);
}

static int	setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return 0;
}

void	Server::registerToEpoll(int fd, int epoll_event) //take this out of here, used by cgi aswell
{
	if (setNonBlocking(fd) == -1)
		throw std::runtime_error("fcntl() error");
	struct epoll_event ev;
	ev.events = epoll_event;
	ev.data.fd = fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		throw std::runtime_error("epoll_ctl() error");
		// Close fd in call if fails, server fd should stay open
}

Parser::ServerConfig &Server::getConf(int fd)
{
	return (server_conf[client_to_conf[fd]]);
}
