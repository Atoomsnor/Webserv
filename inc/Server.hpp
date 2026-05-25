#include "Parser.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>

class Server
{
	private:
		std::vector<Parser::ServerConfig> server_conf;
		int	epoll_fd;
		int server_fd;
		sockaddr_in server_addr;
		struct epoll_event ev;
		struct epoll_event events[64];

		Server() = delete;
		Server &operator=(const Server &rhs) = delete;
		Server(const Server &cpy) = delete;
	public:
		Server(std::vector<Parser::ServerConfig> server_conf);
		Server &operator=(const Server &&rhs);
		Server(const Server &&old);
		~Server();
		
		// std::vector<Parser::ServerConfig> &getServerConf() const;

		void loop();
		int setNonBlocking(int fd);

		void print_server(Server &server) const;
};