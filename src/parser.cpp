#include "parser.hpp"
#include "Logger.hpp"
#include <fstream>
#include <iostream>


std::vector<std::string>	Parser::tokenize(const std::string& filepath)
{
	//fileparse
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	std::ifstream file(filepath);
	std::vector<std::string> tokens;
	std::string line;
	std::string cur;

	
	while (std::getline(file, line))
	{
		for (size_t i = 0; i < line.size(); i++)
		{
			char c = line[i];
			if (c == '{' || c == '}' || c == ';')
			{
				if (!cur.empty())
				{
					tokens.push_back(cur);
					cur.clear();
				}
				tokens.push_back(std::string(1, c));
				lp->printLog("line: {}", tokens);
			}
			else if (std::isspace(c))
			{
				if (!cur.empty())
				{
					tokens.push_back(cur);
					cur.clear();
				}
			}
			else
			cur += c;
		}
	}
	if (!cur.empty())
		tokens.push_back(cur);
	return (tokens);
}






// static std::vector<ServerConfig>	parse(const std::string& filepath);