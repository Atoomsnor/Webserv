#include "Logger.hpp"
#include "parser.hpp"
#include <iostream>

void subf2(int a, std::string b)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	lp->printLog("Skill Rating: {}, Match Making Rating: {}", a, b);
}

int main(void)
{
	// subf2(5, "69");
	// subf2(-5, "Bronze");
	// subf2(696969, "GM");
	std::cout << DEBUG_LOG << std::endl;
	#if DEBUG_LOG == 1
		subf2(5, "69");
	#endif

	std::vector<std::string> tokens = Parser::tokenize("webserv.conf");
	// std::vector<ServerConfig> configs = Parser::parse(tokens);
}