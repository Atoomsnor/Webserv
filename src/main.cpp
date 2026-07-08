#include "Logger.hpp"
#include "Server.hpp"
#include "Parser.hpp"
#include <iostream>
#include <stdexcept>

void print_configs(Parser::ServerConfig &config) // never used? Only for debugging?
{
	auto &err = config.error_pages;
	Logger::printLog("port: {}\nhost: {}\nserver_name: {}\nmax_body_size {}", config.port, config.host, config.server_name, config.max_body_size);
	Logger::printLog("404: {}\n405: {}\n502: {}\n503: {}", err[404], err[405], err[502], err[503]);
	for (auto &loc : config.locations)
	{
		Logger::printLog("path: {}\nroot: {}\nindex: {}\nautoindex: {}\nupload_store: {}\nreturn_code: {}\nreturn_url: {}",
			loc.path, loc.root, loc.index, loc.auto_index, loc.upload_store, loc.return_code, loc.return_url);
		for (auto &method : loc.methods)
			Logger::printLog("method: {}", method);
		for (auto &[ext, bin] : loc.cgi)
			Logger::printLog("cgi: {} -> {}", ext, bin);
		Logger::printLog("\n");
	}
}

int main(int argc, char **argv)
{
	std::vector<std::string> 			tokens; // raw accessing argv[1] was unsafe
	std::vector<Parser::ServerConfig>	configs;
	try
	{
		if (argc < 2)
			tokens = Parser::tokenize("webserv.conf");
		else
			tokens = Parser::tokenize(argv[1]);
		configs = Parser::parse(tokens);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return (1);
	}
	if (configs.empty()) // TODO: as mentioned in parser::parse, check for individual missing tokens instead of empty, cause some might trigger.
	{
		std::cerr << "Config parsing error" << std::endl;
		return (1);
	}
	Server server(configs);
	try {
		server.setup();
		server.print_server(server);
		server.eventLoop();
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return (1);
	}
}