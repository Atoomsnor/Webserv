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

std::vector<std::string>	Server::buildEnv(int fd, HTTP::Request &req, const Parser::LocationConfig &loc)
{
	std::vector<std::string>	envs;

	for (const auto &h : req.headers)
		Logger::printLog("{}: {}", h.first, h.second);

	envs.push_back("AUTH_TYPE="); // absolete, no auth

	if (req.body.empty()) {
		envs.push_back("CONTENT_LENGTH=");
		envs.push_back("CONTENT_TYPE=");
	} else {
		envs.push_back("CONTENT_LENGTH=" + std::to_string(req.body.size())); // y
		envs.push_back("CONTENT_TYPE=" + req.headers["Content-Type"]);
	}
	
	envs.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envs.push_back("PATH_INFO="); // absolete, no trailing path after script
	envs.push_back("PATH_TRANSLATED=" + loc.root + req.uri.substr(loc.path.size()));
	envs.push_back("QUERY_STRING=" + req.query); // y
	envs.push_back("REMOTE_ADDR=" + client_ips[fd]);
	envs.push_back("REMOTE_HOST=" + client_ips[fd]); // We aint doing fancy dns loop, subject says its ok
	envs.push_back("REMOTE_IDENT="); // optional and absolete
	envs.push_back("REMOTE_USER="); // absolete, no auth
	envs.push_back("REQUEST_METHOD=" + req.method); // y
	envs.push_back("SCRIPT_NAME=" + req.uri);
	envs.push_back(std::string("SERVER_NAME=") + inet_ntoa(server_addr.sin_addr)); // idk lmao
	envs.push_back("SERVER_PORT=" + std::to_string(ntohs(server_addr.sin_port)));
	envs.push_back("SERVER_PROTOCOL=HTTP/1.1");
	envs.push_back("SERVER_SOFTWARE=Webserv/1.0");

	for (auto &[key, value] : req.headers)
	{
		if (key == "Content-Type" || key == "Content-Length")
			continue;
		std::string env_key = "HTTP_";
		for (char c : key)
		{
			if (c == '-')
				env_key += '_';
			else
				env_key += std::toupper(static_cast<unsigned char>(c));
		}
		envs.push_back(env_key + '=' + value);
	}
	return (envs);
}

void	Server::handleCGI(int fd, HTTP::Request &req, Parser::LocationConfig *loc, std::string interpreter)
{
	std::string script = req.uri.substr(loc->path.size());
	std::string filepath = "." + loc->root + script;
	
	Logger::printLog("We are in CGI {} {} {} {}", fd, req.uri, filepath, interpreter);
	int		in_pipe[2];
	int		out_pipe[2];

	std::vector<std::string> envStrings = buildEnv(fd, req, *loc); // request to env for execve
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
	int status;

	Logger::printLog("we in CGIResponse for pipe_fd {}", pipe_fd);
	CGIState &state = cgi_states[pipe_fd];

	waitpid(state.pid, &status, 0);

	if (WIFSIGNALED(status))
		sendError(state.client_fd, 500);
	else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		sendError(state.client_fd, 404);

	std::string output;
	char buf[4096];
	ssize_t n;
	while ((n = read(pipe_fd, buf, sizeof(buf))) > 0)
		output.append(buf, n);

	std::string response = "HTTP/1.1 200 OK\r\n" + output;
	send(state.client_fd, response.c_str(), response.size(), 0);

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_fd, nullptr);
	close(pipe_fd);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, state.client_fd, nullptr);
	close(state.client_fd);
	cgi_states.erase(pipe_fd);
}