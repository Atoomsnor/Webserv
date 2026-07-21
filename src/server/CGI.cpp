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

static void	closePipes(int in_pipe[2], int out_pipe[2])
{
	close(in_pipe[0]);
	close(in_pipe[1]);
	close(out_pipe[0]);
	close(out_pipe[1]);
}

void	Server::handleCGI(int fd, HTTP::Request &req, Parser::LocationConfig *loc, std::string interpreter)
{
	std::string script = req.uri.substr(loc->path.size());
	std::string filepath = "." + loc->root + script;
	
	// Logger::printLog("We are in CGI {} {} {} {}", fd, req.uri, filepath, interpreter);
	int		in_pipe[2];
	int		out_pipe[2];

	std::vector<std::string> envStrings = buildEnv(fd, req, *loc); // request to env for execve
	// for (const std::string &e : envStrings)
	// 	Logger::printLog("env: {}", e);

	if (pipe(in_pipe) < 0)
	{
		sendError(fd, 500);
		return ;
	}
	if (pipe(out_pipe) < 0)
	{
		close(in_pipe[0]);
		close(in_pipe[1]);
		sendError(fd, 500);
		return ;
	}

	registerToEpoll(out_pipe[0], EPOLLIN); // watch for CGI script output
	Logger::printLog("registering out_pipe[0] {} for read", out_pipe[0]);
	pid_t pid = fork();
	if (pid == -1)
	{
		closePipes(in_pipe, out_pipe);
		sendError(fd, 500);
		return ;
	}
	if (pid == 0)
	{
		int devnull = open("/dev/null", O_WRONLY);
		if (dup2(in_pipe[0], STDIN_FILENO) == -1 || dup2(out_pipe[1], STDOUT_FILENO) == -1 || dup2(devnull, STDERR_FILENO) == -1)
			exit(1);
		close(devnull);
		closePipes(in_pipe, out_pipe);

		// envStrings to envp for execve()
		std::vector<char*> envp;
		for (std::string &e : envStrings)
			envp.push_back(e.data());
		envp.push_back(nullptr);

		char *argv[] = { interpreter.data(), filepath.data(), nullptr }; // full script path
		execve(interpreter.c_str(), argv, envp.data());
		Logger::printLog("execve failed: {}", strerror(errno));
		exit(1);
	}
	// parent
	close(in_pipe[0]); // child read-end
	close(out_pipe[1]); // child write-end

	int write_fd = -1;
	if (!req.body.empty()) // POST body needs writing to child stdin
	{
		fcntl(in_pipe[1], F_SETFL, O_NONBLOCK);
		registerToEpoll(in_pipe[1], EPOLLOUT);
		write_fd = in_pipe[1];
	}
	else
		close(in_pipe[1]);

	cgi_states[out_pipe[0]] = {fd, out_pipe[0], write_fd, pid, req.body, ""};
}

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
	envs.push_back(std::string("SERVER_NAME=") + inet_ntoa(server_addr.sin_addr)); // TODO << is being overriden with 2 or more servers
	envs.push_back("SERVER_PORT=" + std::to_string(ntohs(server_addr.sin_port))); // TODO << is being overriden with 2 or more servers
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

void	Server::CGIWrite(CGIState &cgi)
{
	ssize_t n = write(cgi.write_fd, cgi.body.data() + cgi.body_sent, cgi.body.size() - cgi.body_sent);
	if (n > 0)
		cgi.body_sent += static_cast<size_t>(n);
	if (n < 0)
		return ;
	if (cgi.body_sent >= cgi.body.size())
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi.write_fd, nullptr);
		close(cgi.write_fd);
		cgi.write_fd = -1;
	}
}

void Server::CGIResponse(CGIState &cgi) //temp
{
	char	buf[4096];
	ssize_t	n = read(cgi.read_fd, buf, sizeof(buf));

	if (n > 0)
	{
		cgi.output.append(buf, n);
		return ; // epoll fires again for the rest
	}
	if (n < 0)
		return ; // can't inspect errno, so treat as "not ready", retry

	int			client_fd = cgi.client_fd;
	std::string	output = cgi.output;

	cleanupCGI(cgi); // closes both pipes, reaps child, erases from cgi_states

	if (output.empty())
		sendError(client_fd, 500);
	else
		sendResponse(client_fd, "HTTP/1.1 200 OK\r\n" + output);
}

void	Server::cleanupCGI(CGIState &cgi)
{
	if (cgi.write_fd >= 0)
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi.write_fd, nullptr);
		close(cgi.write_fd);
	}
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi.read_fd, nullptr);
	close(cgi.read_fd);
	if (cgi.pid > 0)
		waitpid(cgi.pid, nullptr, 0);
	cgi_states.erase(cgi.read_fd); // MUST be last — invalidates cgi
}