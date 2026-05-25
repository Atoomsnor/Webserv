#include "Server.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Server::Server (std::vector<Parser::ServerConfig> server_conf) :
server_conf(server_conf),
epoll_fd(epoll_create1(0)),
server_fd(socket(AF_INET, SOCK_STREAM, 0))
{
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_conf[0].port);
	// Logger::printLog("sin_port: {} ---- [0].port: {}", server_addr.sin_port, server_conf[0].port);
	inet_pton(AF_INET, server_conf[0].host.c_str(), &server_addr.sin_addr);
	bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	listen(server_fd, SOMAXCONN);
	setNonBlocking(server_fd);

	ev.events = EPOLLIN;
	ev.data.fd = server_fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
	// Logger::printLog("server_fd: {}", server_fd);
}

Server &Server::operator=(const Server &&rhs)
{
	if (&rhs != this)
		server_conf = std::move(rhs.server_conf);
	return (*this);	
}

Server::Server(const Server &&old) : server_conf(std::move(old.server_conf))
{}

Server::~Server()
{}

int Server::setNonBlocking(int fd)
{
    int opt = 1;
    if (ioctl(fd, FIONBIO, &opt) == -1)
    {
        perror("ioctl");
        return -1;
    }
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
			if (events[i].data.fd == server_fd)
				acceptClient(events[i].data.fd);
			else
				handleClient(events[i].data.fd);
		}
	}
}

// std::vector<Parser::ServerConfig> &Server::getServerConf() const
// {
	// return (server_conf);
// }

void Server::print_server(Server &server) const
{
	Logger::printLog("epoll_fd: {}\nserver_fd: {}\n", epoll_fd, server_fd);
	Logger::printLog("sin_family {}\nsin_port: {}\nsin_addr {}\n", server_addr.sin_family, server_addr.sin_port, 4);
	(void)server;
}