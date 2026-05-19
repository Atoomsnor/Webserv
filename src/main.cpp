#include "Logger.hpp"
#include "Parser.hpp"
#include <iostream>

void print_configs(ServerConfig &config)
{
	auto &err = config.error_pages;
	Logger::printLog("port: {}\nhost: {}\nserver_name: {}\nmax_body_size {}", config.port, config.host, config.server_name, config.max_body_size);
	Logger::printLog("404: {}\n405: {}\n502: {}\n503: {}", err[404], err[405], err[502], err[503]);
}

int main(void)
{
	std::cout << DEBUG_LOG << std::endl;
	std::vector<std::string> tokens = Parser::tokenize("webserv.conf");
	// for (auto it = tokens.begin(); it != tokens.end(); it++)
		// lp->printLog("token: {}", *it);
	std::vector<ServerConfig> configs = Parser::parse(tokens);
	debug_log(print_configs, configs[0]);
	debug_log(print_configs, configs[1]);
}