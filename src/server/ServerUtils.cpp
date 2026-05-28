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

int	Server::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		return -1;
	return 0;
}

void	Server::print_server(Server &server) const
{
	Logger::printLog("epoll_fd: {}\nsocket_fd: {}\n", epoll_fd, socket_fd);
	Logger::printLog("sin_family {}\nsin_port: {}\nsin_addr {}\n", server_addr.sin_family, server_addr.sin_port, 4);
	(void)server;
}