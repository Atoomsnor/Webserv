#pragma once

#include <string>
#include <vector>
#include <map>

struct ServerConfig {
	int							port;
	std::string					host;
	std::string					server_name;
	std::map<int, std::string>	error_pages;
	std::size_t					max_body_size;
	// std::vector<LocationConfig>	locations;
};

class Parser {
	private:

	public:
		static std::vector<std::string>		tokenize(const std::string& filepath);
		static std::vector<ServerConfig>	&parse(std::vector<std::string> tokens);
};

// struct LocationConfig {

// };