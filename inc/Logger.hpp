#pragma once

#include <fstream>
#include <memory>
#include <format>
#include <string>

class Logger {
	private:
		static std::unique_ptr<Logger> instance;
		std::unique_ptr<std::ofstream> file;
		std::string	get_time(void) const;
		Logger(); 
	public:
		static std::unique_ptr<Logger> &getInstance();

		Logger(Logger &cpy) = delete;
		void operator=(const Logger &cpy) = delete;

	template<typename... Args>
	void printLog(const std::string& format_str, Args&&... args) const {
		*file << std::format("{}: {}", get_time(), std::vformat(format_str, std::make_format_args(args...))) << std::endl;
	}
};