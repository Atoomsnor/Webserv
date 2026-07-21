#pragma once

#include "HTTP.hpp"
#include "Parser.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

struct CGIState
{
	int			client_fd;
	int			read_fd; // out_pipe[0], cgi_states key
	int			write_fd; // in_pipe[1]
	pid_t		pid;
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
		std::map<int, std::string>			client_ips; // client_fd -> client IP, for logging and CGI env vars
		std::map<int, CGIState>				cgi_states; // out_pipe[0] -> CGI state, for building the response
		std::map<int, std::string>			client_buffers; // client_fd -> accumulated bytes until a full request is read
		std::map<int, std::string>			pending_sends;
		std::map<int, size_t>				socket_to_conf;
		std::map<int, size_t>				client_to_conf;

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
		int							createSocket();
		void						bindAndListen(int fd, size_t i);
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

		CGIState				*findCGI(int fd);
		void					cleanupCGI(CGIState &cgi);
		void					CGIWrite(CGIState &cgi);
		void					CGIResponse(CGIState &cgi);

		void						sendResponse(int fd, const std::string &response);
		void						flushPending(int fd);

		Parser::ServerConfig &getConf(int fd);
};