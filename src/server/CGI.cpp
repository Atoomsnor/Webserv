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
#include <sys/wait.h>

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

	int		in_pipe[2];
	int		out_pipe[2];

	std::vector<std::string> envStrings = buildEnv(req, filepath); // request to env for execve
	for (const std::string &e : envStrings)
		Logger::printLog("env: {}", e);

	if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0)
	{
		sendError(fd, 500);
		return ;
	}

	registerToEpoll(out_pipe[0], EPOLLIN); // watch for CGI script output
	Logger::printLog("registering out_pipe[0] {} for read", out_pipe[0]);
	pid_t pid = fork();
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
		close(in_pipe[1]);
		close(out_pipe[0]);
		if (dup2(in_pipe[0], STDIN_FILENO) == -1 || dup2(out_pipe[1], STDOUT_FILENO) == -1)
			exit(1);
		close(in_pipe[0]);
		close(out_pipe[1]);

		std::vector<char*> envp;
		for (std::string &e : envStrings)
			envp.push_back(e.data());
		envp.push_back(nullptr);

		char *argv[] = { interpreter.data(), filepath.data(), nullptr };
		execve(interpreter.c_str(), argv, envp.data());
		Logger::printLog("execve failed: {}", strerror(errno));
		exit(1);
	}
	// parent
	Logger::printLog("Hello im a dad");

	close(in_pipe[0]); // child read-end
	close(out_pipe[1]); // child write-end

	if (!req.body.empty()) // POST body needs to be written to child stdin
	{
		Logger::printLog("registering in_pipe[1] {} for write", in_pipe[1]);
		registerToEpoll(in_pipe[1], EPOLLOUT); // watch for when pipe is ready to write
		cgi_write[in_pipe[1]] = out_pipe[0]; // link write fd back to read fd for state lookup
	}
	else
		close(in_pipe[1]);

	cgi_states[out_pipe[0]] = {fd, pid, in_pipe[1], req.body}; // store state for when epoll fires
}

void	Server::CGIWrite(int pipe_fd)
{
	Logger::printLog("we in cgiwrite");
	int	out_fd = cgi_write[pipe_fd];
	CGIState &state = cgi_states[out_fd];

	write(pipe_fd, state.body.c_str(), state.body.size()); //errorcheck
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_fd, nullptr);
	close(pipe_fd);
	cgi_write.erase(pipe_fd);
}

void Server::CGIResponse(int pipe_fd) //temp
{
	Logger::printLog("we in CGIResponse for pipe_fd {}", pipe_fd);
    CGIState &state = cgi_states[pipe_fd];

    std::string output;
    char buf[4096];
    ssize_t n;
    while ((n = read(pipe_fd, buf, sizeof(buf))) > 0)
        output.append(buf, n);

    waitpid(state.pid, nullptr, 0);

    std::string response = "HTTP/1.1 200 OK\r\n" + output;
    send(state.client_fd, response.c_str(), response.size(), 0);

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_fd, nullptr);
    close(pipe_fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state.client_fd, nullptr);
    close(state.client_fd);
    cgi_states.erase(pipe_fd);
}