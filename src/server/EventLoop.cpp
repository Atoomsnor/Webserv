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

void	Server::eventLoop()
{
	int n;
	while (true)
	{
		// im epolling it up rn
		n = epoll_wait(epoll_fd, events, 64, 1000);
		if (n < 0)
			throw std::runtime_error("epoll_wait() error");
		for (int i = 0; i < n; i++)
		{
			// Logger::printLog("epoll fired on fd {}", events[i].data.fd);
			if (cgi_write.count(events[i].data.fd))
				CGIWrite(events[i].data.fd);
			else if (cgi_states.count(events[i].data.fd))
				CGIResponse(events[i].data.fd);
			else if (events[i].data.fd == socket_fd)
				acceptClient(events[i].data.fd);
			else
				handleClient(events[i].data.fd);
		}
	}
}

void	Server::acceptClient(int fd)
{
	sockaddr_in	client_addr;
	socklen_t	addr_len = sizeof(client_addr);
	int			client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
	client_ips[client_fd] = inet_ntoa(client_addr.sin_addr);

	if (client_fd < 0)
		return ;
	try {
		registerToEpoll(client_fd, EPOLLIN);
	} catch (...) {
		close(client_fd);
		Logger::printLog("closed fd: {}", client_fd);
		throw ;
	}
	Logger::printLog("New client at fd: {} and ip: {}", client_fd, client_ips[client_fd]);
}
