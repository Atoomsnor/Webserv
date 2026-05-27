#include "Server.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server (std::vector<Parser::ServerConfig> server_conf) :
	server_conf(server_conf),
	epoll_fd(-1),
	socket_fd(-1)
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		perror("epoll_create1() error");
		// throw std::exception();
	}

	socket_fd = socket(AF_INET, SOCK_STREAM, 0)
	if (socket_fd == -1)
	{
		perror ("socket() error");
		// throw std::exception();
	}

	int opt = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		perror ("setsockopt() error");
		close (socket_fd);
		// throw std::exception();
	}

	// Setup server_addr (prob make function for this);
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_conf[0].port);
	if (inet_pton(AF_INET, server_conf[0].host.c_str(), &server_addr.sin_addr) != 1)
	{
		perror ("inet_pton() error");
		// throw std::exception();
	}

	// Logger::printLog("sin_port: {} ---- [0].port: {}", server_addr.sin_port, server_conf[0].port);
	if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		perror ("bind() error");
		// throw std::exception();
	}

	if (listen(socket_fd, SOMAXCONN) == -1)
	{
		perror ("listen() error");
		// throw std::exception();
	}

	if (setNonBlocking(socket_fd) == -1)
	{
		perror ("ioctl() error");
		// throw std::exception();
	}

	ev.events = EPOLLIN;
	ev.data.fd = socket_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
	{
		perror ("epoll_ctl() error");
		// throw std::exception();
	}
	// Logger::printLog("socket_fd: {}", socket_fd);
}


int Server::setNonBlocking(int fd)
{
	int opt = 1;
	if (ioctl(fd, FIONBIO, &opt) == -1)
	return -1;
	return 0;
}

void acceptClient(int fd)
{
	(void)fd;
}

void handleClient(int fd)
{
	(void)fd;
}

void Server::loop()
{
	int n;
	while (true)
	{
		// im epolling it up rn
		n = epoll_wait(epoll_fd, events, 64, -1);
		if (n < 0)
		throw std::exception();
		for (int i = 0; i < n; i++)
		{
			if (events[i].data.fd == socket_fd)
				acceptClient(events[i].data.fd);
			else
				handleClient(events[i].data.fd);
		}
	}
}

// std::vector<Parser::ServerConfig> &Server::getServerConf() const
// {
// 	return (server_conf);
// }
	
void Server::print_server(Server &server) const
{
	Logger::printLog("epoll_fd: {}\nsocket_fd: {}\n", epoll_fd, socket_fd);
	Logger::printLog("sin_family {}\nsin_port: {}\nsin_addr {}\n", server_addr.sin_family, server_addr.sin_port, 4);
	(void)server;
}

Server &Server::operator=(const Server &&rhs)
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

Server::Server(const Server &&old) : server_conf(std::move(old.server_conf))
{}

Server::~Server()
{
	if (epoll_fd >= 0)
		close (epoll_fd);
	if (socket_fd >= 0)
		close (socket_fd);
}