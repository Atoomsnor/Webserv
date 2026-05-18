#include "parser.hpp"
#include "Logger.hpp"
#include <fstream>
#include <iostream>


std::vector<std::string>	Parser::tokenize(const std::string& filepath)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	std::ifstream file(filepath);
	std::string line;
	std::vector<std::string> ret;
	while (std::getline(file, line))
	{
		lp->printLog("line: {}", line);
		ret.push_back(line + '\n');
	}
	return (ret);
}





// static std::vector<ServerConfig>	parse(const std::string& filepath);