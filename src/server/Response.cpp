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

void Server::sendResponse(int fd, const std::string &response)
{
	pending_sends[fd] = response;
	// change epoll to watch for writability
	struct epoll_event ev;
	ev.events = EPOLLOUT;
	ev.data.fd = fd;
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void Server::flushPending(int fd)
{
	std::string &data = pending_sends[fd];
	ssize_t sent = send(fd, data.c_str(), data.size(), 0);
	if (sent < 0)
	{
		Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
		pending_sends.erase(fd);
		client_buffers.erase(fd);
		client_ips.erase(fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return;
	}
	if (sent > 0)
		data = data.substr(sent);
	if (data.empty())
	{
		pending_sends.erase(fd);
		client_buffers.erase(fd);
		client_ips.erase(fd);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
	}
}