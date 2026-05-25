#include "Parser.hpp"

class Server
{
	private:
		std::vector<ServerConfig> server_conf;
		Server() = delete;
		Server &operator=(const Server &rhs) = delete;
		Server(const Server &cpy) = delete;
	public:
		Server (std::vector<ServerConfig> server_conf);
		Server &operator=(const Server &&rhs);
		Server(const Server &&old);
		
		std::vector<ServerConfig> &getServerConf() const;
};