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

Server::Server (std::vector<Parser::ServerConfig> server_conf) :
	server_conf(server_conf),
	epoll_fd(-1),
	socket_fd(-1) {}
	
void	Server::setup()
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
		throw std::runtime_error("epoll_create1() error");
	
	socketSetup();
	
	Logger::printLog("socket_fd: {}", socket_fd);
}

// std::vector<Parser::ServerConfig> &Server::getServerConf() const
// {
// 	return (server_conf);
// }
	
Server	&Server::operator=(const Server &&rhs)
{
	if (&rhs != this)
	{
		server_conf	= std::move(rhs.server_conf);
		epoll_fd	= rhs.epoll_fd;
		socket_fd	= rhs.socket_fd;
		// put new fd's to -1 aswell???
	}
	return (*this);	
}

Server::Server(const Server &&old) :
	server_conf(std::move(old.server_conf)) {}

Server::~Server()
{
	if (epoll_fd >= 0)
		close(epoll_fd);
	if (socket_fd >= 0)
		close(socket_fd);
}
