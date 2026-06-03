#pragma once

#include <string>
#include <map>

namespace HTTP
{
	struct Request {
		std::string							method;
		std::string							uri;
		// std::string							version;
		std::string							body;
		std::map<std::string, std::string>	headers;
	};

	// struct postData
	// {
	// 	std::string type_name;
	// 	std::string file_name;
	// 	std::string body;
	// };

	Request	parse(const std::string &raw);
	std::string getResponseCode(int code);
	// postData	getPostData(const std::string data);

	std::string buildResponse(const size_t size, const std::string &body,
		const std::string code, const std::string &content_type);
}