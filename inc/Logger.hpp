#pragma once

#include <fstream>
#include <memory>
#include <format>
#include <string>

#ifndef DEBUG_LOG
# define DEBUG_LOG 0
#endif

class Logger {
	private:
		static std::unique_ptr<Logger> instance;
		std::unique_ptr<std::ofstream> file;
		std::string	get_time(std::string str) const;
		Logger(); 
	public:
		static std::unique_ptr<Logger> &getInstance();

		Logger(Logger &cpy) = delete;
		void operator=(const Logger &cpy) = delete;

	template<typename... Args>
	void printLog(const std::string& format_str, Args&&... args) const {
		#if DEBUG_LOG == 1
			*file << std::format("{} {}", get_time("%H:%M:%S"), std::vformat(format_str, std::make_format_args(args...))) << std::endl;
		#endif
	}
};