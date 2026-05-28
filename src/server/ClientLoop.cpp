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

void	Server::handleClient(int fd)
{
	char buf[4096];
	ssize_t bytes = recv(fd, buf, sizeof(buf), 0);
	if (bytes <= 0)
	{
		if (bytes < 0)
			perror("recv() error"); // should be ok
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		perror("geuss im here now xd"); // need to be changed oneday
		return ;
	}
	Logger::printLog("received {} bytes", bytes);
	
	std::ifstream fs(server_conf[0].locations[0].index);
	std::string str;

	if (!fs)
		str = "<html><body><p>Hello World!</p></body></html>";
	else {
		std::stringstream buffer;
		buffer << fs.rdbuf();
		str = buffer.str();
	}
	std::string response =  "HTTP/1.1 200 OK\r\n"
							"Content-Length: ";
							response += std::to_string(str.size());
							response += "\r\n\r\n";
							response += str;
	send(fd, response.c_str(), response.size(), 0);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}