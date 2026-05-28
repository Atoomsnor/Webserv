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
	createSocket();
	bindAndListen();
	registerToEpoll(socket_fd);
}

void	Server::createSocket()
{
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
		throw std::runtime_error("socket() error");
	
	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		close(socket_fd);
		throw std::runtime_error("setsockopt() error");
	}
}

void	Server::bindAndListen()
{
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_conf[0].port);
	if (inet_pton(AF_INET, server_conf[0].host.c_str(), &server_addr.sin_addr) != 1)
		throw std::runtime_error("inet_pton() error");

	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		throw std::runtime_error("bind() error");
		
	if (listen(socket_fd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() error");

	Logger::printLog("sin_port: {} ---- [0].port: {}", server_addr.sin_port, server_conf[0].port);
}

void	Server::registerToEpoll(int fd)
{
	if (setNonBlocking(fd) == -1)
		throw std::runtime_error("fcntl() error");
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
		throw std::runtime_error("epoll_ctl() error");
		// Close fd in call if fails, server fd should stay open
}