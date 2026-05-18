#include "parser.hpp"
#include <fstream>
#include <iostream>


std::vector<std::string>	Parser::tokenize(const std::string& filepath)
{
	std::ifstream file(filepath);
	std::string raw, line;
	while (std::getline(file, line))
		raw += line;
	std::cout << raw << std::endl;	return {};}





// static std::vector<ServerConfig>	parse(const std::string& filepath);