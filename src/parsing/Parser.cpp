#include "Parser.hpp"
#include "Logger.hpp"
#include <cctype>
#include <fstream>
#include <iostream>

std::vector<std::string>	Parser::tokenize(const std::string &filepath)
{
	//fileparse
	std::ifstream file(filepath);
	std::vector<std::string> tokens;
	std::string cur;

	auto flush = [&]() {
		if (!cur.empty()) {
			tokens.push_back(std::move(cur));
			cur.clear();
		}
	};

	char	c;
	while (file.get(c))
	{
		if (c == '{' || c == '}' || c == ';') {
			flush();
			tokens.push_back(std::string(1, c));
		}
		else if (std::isspace(c))
			flush();
		else
			cur += c;
	}
	flush();
	return (tokens);
}

static int	count_servers(std::vector<std::string> &tokens)
{
	int count = 0;
	for (std::size_t i = 0; i + 1 < tokens.size(); ++i)
	{
		if (tokens[i] == "server" && tokens[i + 1] == "{")
			++count;
	}
	return (count);
}

std::map<int, std::string>	init_errorpages(void)
{
	std::map<int, std::string> map;
	for (int i = 400; i <= 451; i++)
		map.insert({i, "/index.html"});
	for (int i = 500; i <= 511; i++)
		map.insert({i, "/index.html"});
	return (map);
}

std::vector<Parser::ServerConfig>	Parser::parse(std::vector<std::string> &tokens)
{
	return (serverParse(tokens));
}

static void	extract_ip(Parser::ServerConfig &sc, std::string str)
{
	size_t split_pos = str.find(':');
	if (split_pos == str.npos)
		return ; // throw exception?
	sc.host = str.substr(0, split_pos);
	size_t prev = 0;
	for (int i = 0; i < 4; i++)
	{
		size_t next = sc.host.find('.', prev + 1);
		if (next == sc.host.npos && i == 3)
			next = sc.host.length();
		else if (next == sc.host.npos)
			Logger::printLog("ERROR A IP: {}", sc.host), throw std::exception();
		int size = std::stoi(sc.host.substr(prev, next));
		if (size < 0 || size >= 256)
			Logger::printLog("ERROR B IP: {}", sc.host), throw std::exception();
		prev = next + 1;
	}
	sc.port = std::stoi(str.substr(split_pos + 1, str.length() - split_pos));
}

std::vector<Parser::ServerConfig>	Parser::serverParse(std::vector<std::string> &tokens)
{
	std::vector<Parser::ServerConfig>	sc(count_servers(tokens));
	int							server_idx = -1;

	for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); it++)
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
			extract_ip(sc[server_idx], *(it + 1));
		else if (*it == "server_name")
			sc[server_idx].server_name = *(it + 1);
		else if (*it == "client_max_body_size")
			sc[server_idx].max_body_size = std::stoi(*(it + 1));
		else if (*it == "error_page")
			sc[server_idx].error_pages[std::stoi(*(it + 1))] = *(it + 2);
		else if (*it == "location")
			sc[server_idx].locations.push_back(locationParse(it, tokens.end()));
		// Logger::printLog("token: {}", *it);
	}
	return (sc);
}

Parser::LocationConfig	Parser::locationParse(std::vector<std::string>::iterator &it,
								std::vector<std::string>::iterator end)
{
	Parser::LocationConfig lc;

	lc.path = *(it + 1);
	it += 3;
	while (it != end && *it != "}")
	{
		if (*it == "methods")
		{
			++it;
			while (it != end && *it != ";")
			{
				lc.methods.push_back(*it);
				++it;
			}
		}
		else if (*it == "root")
			lc.root = *(it + 1);
		else if (*it == "index")
			lc.index = *(it + 1);
		else if (*it == "autoindex")
			lc.auto_index = (*(it + 1) == "on");
		else if (*it == "upload_store")
			lc.upload_store = *(it + 1);
		else if (*it == "return")
		{
			lc.return_code = std::stoi(*(it + 1));
			lc.return_url = *(it + 2);
		}
		else if (*it == "cgi")
			lc.cgi[*(it + 1)] = *(it + 2);
		++it;
	}
	return lc;
}
