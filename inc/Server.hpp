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
		struct epoll_event					ev;
		struct epoll_event					events[64];

	public:
		Server(std::vector<Parser::ServerConfig> server_conf);
		Server &operator=(const Server &&rhs);
		Server(const Server &&old);
		~Server();
		
		// std::vector<Parser::ServerConfig> &getServerConf() const;
		void	createSocket();
		void	loop();
		void	registerToEpoll(int fd);
		int		setNonBlocking(int fd);
		void	bindAndListen();
		void	acceptClient(int fd);
		void	handleClient(int fd);
		void	print_server(Server &server) const;
};