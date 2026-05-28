#pragma once

#include <string>

namespace HTTP
{
	std::string getResponseCode(int code);
	std::string buildHTTPResponse(const size_t size, const std::string &body,
		const std::string code, const std::string &content_type);
}