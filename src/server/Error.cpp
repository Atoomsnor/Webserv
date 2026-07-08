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

void	Server::sendError(int fd, int error_code)
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
	if (send(fd, response.c_str(), response.size(), 0) == -1)
		//TODO: check errno and act according
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	fs.close();
	client_buffers.erase(fd);
	close(fd);
}