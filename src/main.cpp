#include "Logger.hpp"
#include "Parser.hpp"
#include <iostream>

void subf2(int a, std::string b)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	lp->printLog("Skill Rating: {}, Match Making Rating: {}", a, b);
}

void print_configs(std::vector<ServerConfig> &configs)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	auto &err = configs[0].error_pages;
	lp->printLog("port: {}\nhost: {}\nserver_name: {}\nmax_body_size {}", configs[0].port, configs[0].host, configs[0].server_name, configs[0].max_body_size);
	lp->printLog("404: {}\n405: {}\n502: {}\n503: {}", err[404], err[405], err[502], err[503]);
}

int main(void)
{
	// subf2(5, "69");
	// subf2(-5, "Bronze");
	// subf2(696969, "GM");
	// std::unique_ptr<Logger>& lp = Logger::getInstance();
	std::cout << DEBUG_LOG << std::endl;
	std::vector<std::string> tokens = Parser::tokenize("testserv.conf");
	// for (auto it = tokens.begin(); it != tokens.end(); it++)
		// lp->printLog("token: {}", *it);
	std::vector<ServerConfig> configs = Parser::parse(tokens);
	debug_log(print_configs, configs);
}