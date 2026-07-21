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
	std::string filepath = "." + getConf(fd).error_pages[error_code];
	std::ifstream fs(filepath, std::ios::binary);
	std::string str;
	if (!fs)
	{
		Logger::printLog("Failed to open file {}", filepath);
		str = HTTP::getResponseCode(error_code);
	}
	else
	{
		std::stringstream ss_buffer;
		ss_buffer << fs.rdbuf();
		fs.close();
		str = ss_buffer.str();
	}
	std::string response = HTTP::buildResponse(str.size(), str, HTTP::getResponseCode(error_code), getContentType(filepath));
	Logger::printLog("response: {}", response);
	sendResponse(fd, response);
	// if (send(fd, response.c_str(), response.size(), 0) == -1)
		// Logger::printLog("send() failed on fd {}: {}", fd, strerror(errno));
	// epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	// fs.close();
	client_buffers.erase(fd);
	// close(fd);
}