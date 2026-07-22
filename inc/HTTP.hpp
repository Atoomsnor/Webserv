#pragma once

#include <string>
#include <map>

namespace HTTP
{
	struct postData
	{
		std::string type_name;
		std::string file_name;
		bool empty = 1;
	};

	struct Request {
		std::string							method;
		std::string							uri;
		std::string							version;
		std::string							body;
		std::string							query;
		std::map<std::string, std::string>	headers;
		postData							pd;
	};

	Request		parse(const std::string &raw);
	std::string getResponseCode(int code);
	postData	getPostData(std::istringstream &iss);
	std::string	getPDBody(const std::string &data, size_t max);
	std::string buildAutoindex(std::string filepath);
	std::string getQuery(std::string &uri);
	std::string	buildResponse(const size_t size, const std::string &body, const std::string code, const std::string &content_type, std::string target);
	std::string buildResponse(const size_t size, const std::string &body,
		const std::string code, const std::string &content_type);
}