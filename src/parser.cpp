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
		lp->printLog("line: {}", tokens.back());
	}
	if (!cur.empty())
		tokens.push_back(cur);
	return (tokens);
}

std::vector<ServerConfig>	parse(std::vector<std::string> &tokens)
{
	std::vector<ServerConfig>	sc;
	int							depth_count;
	std::unique_ptr<Logger>& lp = Logger::getInstance();

	for (auto it = tokens.begin(); it != tokens.end(); it++)
	{
		lp->printLog("token: {}", *it);
		if (*it == "listen")
			sc[depth_count].port = *(it + 1);
		else if (*it == '{')
			depth_count++, sc.push_back();
		else if (*it == "host")
			sc[depth_count].host = *(it + 1);
		else if (*it == "server_name")
			sc[depth_count].server_name = *(it + 1);
		else if (*it == "error_page")
			sc[depth_count].error_pages = {std::stoi(*(it + 1)), *(it + 2)};
	}
	return (sc);
}