#include "Server.hpp"
#include "HTTP.hpp"
#include "Parser.hpp"
#include "Logger.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sstream>
#include <stdexcept>

std::vector<std::string>	buildEnv(const HTTP::Request &req, const std::string &filepath)
{
	std::vector<std::string>	envs;

	envs.push_back("REQUEST_METHOD=" + req.method);
	envs.push_back("QUERY_STRING=" + req.query);
	envs.push_back("CONTENT_LENGTH=" + std::to_string(req.body.size()));
	envs.push_back("SCRIPT_FILENAME=" + filepath);
	envs.push_back("PATH_INFO=" + req.uri);

	return (envs);
}


void	Server::handleCGI(int fd, HTTP::Request &req, std::string filepath, std::string interpreter)
{
	Logger::printLog("We are in CGI {} {} {} {}", fd, req.uri, filepath, interpreter);
	
	int		in_pipe[2]; // server -> CGI stdin
	int		out_pipe[2]; // CGI stdout -> server
	pid_t	pid;
	
	std::vector<std::string> envStrings = buildEnv(req, filepath);
	for (const std::string &e : envStrings)
		Logger::printLog("env: {}", e);
	pipe(in_pipe);
	pipe(out_pipe);
	pid = fork();
	if (pid == -1)
	{
		close(in_pipe[0]);
		close(in_pipe[1]);
		close(out_pipe[0]);
		close(out_pipe[1]);
		sendError(fd, 500);
		return ;
	}
	if (pid == 0)
	{
		close(in_pipe[1]); // close stdin write
		close(out_pipe[0]); // close stdout read
		if (dup2(in_pipe[0], STDIN_FILENO) == -1 || dup2(out_pipe[1], STDOUT_FILENO) == -1)
			exit(1);
		close(in_pipe[0]);
		close(out_pipe[1]);

		std::vector<char*> envp;
		for (std::string &e : envStrings)
			envp.push_back(e.data());
		envp.push_back(nullptr);

		char *argv[] = { interpreter.data(), filepath.data(), nullptr};
		execve(interpreter.c_str(), argv, envp.data());
		exit (1);
	}
	else
	{
		Logger::printLog("Hello im a parent");
		close(in_pipe[0]);
		close(out_pipe[1]);
		
	}
}