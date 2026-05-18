#pragma once

#include <string>
#include <vector>
#include <map>

class Parser {
	private:

	public:
		static std::vector<std::string>		tokenize(const std::string& filepath);
		// static std::vector<ServerConfig>	parse(tokens);
};

// struct LocationConfig {

// };

struct ServerConfig {
	int							port;
	std::string					host;
	std::string					serverName;
	std::map<int, std::string>	errorPages;
	std::size_t					maxBodySize;
	// std::vector<LocationConfig>	locations;
};