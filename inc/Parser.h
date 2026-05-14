#pragma once

#include <string>
#include <vector>
#include <map>

struct LocationConfig {

};


struct ServerConfig {
	int							port;
	std::string					host;
	std::string					serverName;
	std::map<int, std::string>	errorPages;
	std::size_t					maxBodySize;
	std::vector<LocationConfig>	locations;
};