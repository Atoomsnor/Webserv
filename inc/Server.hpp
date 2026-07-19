#pragma once

#include "HTTP.hpp"
#include "Parser.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

struct CGIState
{
	int			client_fd;
	int			write_fd; // in_pipe[1]
	std::string	body;
	std::string	output;
};

class Server
{
	private:
		Server() = delete;
		Server &operator=(const Server &rhs) = delete;
		Server(const Server &cpy) = delete;

		std::vector<Parser::ServerConfig>	server_conf;
		int									epoll_fd;
		int									socket_fd;
		sockaddr_in							server_addr;
		struct epoll_event					events[64];
		std::map<int, std::string>			client_ips;
		std::map<int, CGIState>				cgi_states;
		std::map<int, int>					cgi_write; // in_pipe[1] -> out_pipe[0]
		std::map<int, std::string> 			client_buffers; // eyo wtf is this

	public:
		Server(std::vector<Parser::ServerConfig> server_conf);
		Server &operator=(const Server &&rhs);
		Server(const Server &&old);
		~Server();
		
		/* Server */
		void						setup();
		void						print_server(Server &server) const;
		
		/* SocketSetup */
		void						socketSetup();
		void						createSocket();
		void						bindAndListen();
		void						registerToEpoll(int fd, int epoll_event);
		
		/* EventLoop */
		void						eventLoop();
		void						acceptClient(int fd);

		/* Error */
		void						sendError(int fd, int error_code);

		/* HandleClient*/
		void						handleClient(int fd);
		Parser::LocationConfig		*matchLocation(const std::string &uri);
		bool						fetchRequest(int fd);

		/* Handlers */
		std::string					getContentType(const std::string &path);

		void						handleDelete(int fd, std::string uri, Parser::LocationConfig *loc);
		void						handleGet(int fd, std::string uri, Parser::LocationConfig *loc);
		void						handlePost(int fd, std::string &uri, Parser::LocationConfig *loc, HTTP::Request &req);
		void						handleRedir(int fd, std::string &uri, Parser::LocationConfig *loc, HTTP::Request &req);

		/* CGI */
		void						handleCGI(int fd, HTTP::Request &req, Parser::LocationConfig *loc, std::string interpreter);
		std::vector<std::string>	buildEnv(int fd, HTTP::Request &req, const Parser::LocationConfig &loc);

		void						CGIWrite(int pipe_fd);
		void						CGIResponse(int pipe_fd);
};