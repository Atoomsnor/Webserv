#pragma once

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
	std::string							path;
	std::string							root;
	std::string							index;
	bool								auto_index = false;
	std::vector<std::string>			methods;
	std::string							upload_store;
	int									return_code = 0;
	std::string							return_url;
	std::map<std::string, std::string>	cgi;
};

struct ServerConfig {
	int							port;
	std::string					host;
	std::string					server_name; //not needed?
	std::map<int, std::string>	error_pages;
	std::size_t					max_body_size;
	std::vector<LocationConfig>	locations;
};

class Parser {
	private:

	public:
		static std::vector<std::string>		tokenize(const std::string &filepath);
		static std::vector<ServerConfig>	parse(std::vector<std::string> &tokens);
		static std::vector<ServerConfig>	serverParse(std::vector<std::string> &tokens);
		static LocationConfig				locationParse(std::vector<std::string>::iterator &it,
													std::vector<std::string>::iterator end);
};