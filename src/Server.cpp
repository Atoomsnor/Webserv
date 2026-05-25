#include "Server.hpp"


Server::Server (std::vector<ServerConfig> server_conf) : server_conf(server_conf)
{}

Server &Server::operator=(const Server &&rhs)
{
	if (&rhs != this)
		server_conf = std::move(rhs.server_conf);
	return (*this);	
}

Server::Server(const Server &&old) : server_conf(std::move(old.server_conf))
{}

std::vector<ServerConfig> &Server::getServerConf() const
{
	return (server_conf);
}