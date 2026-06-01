#pragma once

#include "HTTP.hpp"
#include "Parser.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

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

	public:
		Server(std::vector<Parser::ServerConfig> server_conf);
		Server &operator=(const Server &&rhs);
		Server(const Server &&old);
		~Server();
		
		void		setup();
		
		void		socketSetup();
		void		createSocket();
		void		bindAndListen();
		void		registerToEpoll(int fd);
		
		void		clientLoop();
		void		acceptClient(int fd);
		void		handleClient(int fd);
		
		int			setNonBlocking(int fd);
		void		print_server(Server &server) const;
		void		sendError(int fd, int error_code);
		void		handleGet(int fd, std::string uri, Parser::LocationConfig *loc);
		void		handlePost(int fd, std::string uri, Parser::LocationConfig *loc, HTTP::postData pd);
		void		handleDelete(int fd, std::string uri, Parser::LocationConfig *loc);
		std::string	getContentType(const std::string &path);

		Parser::LocationConfig	*matchLocation(const std::string &uri);
		// std::vector<Parser::ServerConfig> &getServerConf() const;
	};