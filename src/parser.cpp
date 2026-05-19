#include "Parser.hpp"
#include "Logger.hpp"
#include <cctype>
#include <fstream>
#include <iostream>


std::vector<std::string>	Parser::tokenize(const std::string &filepath)
{
	//fileparse
	// std::unique_ptr<Logger>& lp = Logger::getInstance();
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
		// lp->printLog("line: {}", tokens.back());
	}
	if (!cur.empty())
		tokens.push_back(cur);
	return (tokens);
}

int count_servers(std::vector<std::string> &tokens)
{
	int count = 0;
	for (std::size_t i = 0; i + 1 < tokens.size(); ++i)
	{
		if (tokens[i] == "server" && tokens[i + 1] == "{")
			++count;
	}
	return (count);
}

std::map<int, std::string> init_errorpages(void)
{
	std::map<int, std::string> map;
	for (int i = 400; i <= 451; i++)
		map.insert({i, "/index.html"});
	for (int i = 500; i <= 511; i++)
		map.insert({i, "/index.html"});
	return (map);
}

std::vector<ServerConfig>	Parser::parse(std::vector<std::string> &tokens)
{
	return (serverParse(tokens));
}

std::vector<ServerConfig> Parser::serverParse(std::vector<std::string> &tokens)
{
	std::vector<ServerConfig>	sc(count_servers(tokens));
	std::unique_ptr<Logger>&	lp = Logger::getInstance();
	int							server_idx = -1;

	for (auto it = tokens.begin(); it != tokens.end(); it++)
	{
		if (*it == "server" && *(it + 1) == "{")
		{
			++server_idx;
			sc[server_idx].error_pages = init_errorpages();
			continue ;
		}
		if (server_idx < 0)
			continue ;
		if (*it == "listen")
			sc[server_idx].port = std::stoi(*(it + 1));
		else if (*it == "host")
			sc[server_idx].host = *(it + 1);
		else if (*it == "server_name")
			sc[server_idx].server_name = *(it + 1);
		else if (*it == "client_max_body_size")
			sc[server_idx].max_body_size = std::stoi(*(it + 1));
		else if (*it == "error_page")
			sc[server_idx].error_pages[std::stoi(*(it + 1))] = *(it + 2);
		lp->printLog("token: {}", *it);
	}
	return (sc);
}

// std::vector<LocationConfig>	Parser::locationParse(std::vector<std::string> &tokens)
// {

// }