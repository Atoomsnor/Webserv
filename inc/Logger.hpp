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
		std::unique_ptr<std::ofstream> file;
		std::string	getTime(std::string str) const;
		void createLog(void);
		Logger() = delete;
	public:

		Logger(Logger &cpy) = delete;
		void operator=(const Logger &cpy) = delete;

	template<typename... Args>
	static void printLog(const std::string& format_str, Args&&... args) const
	{
		if (!file)
			createLog();
		*file << std::format("{} {}", get_time("%H:%M:%S"), std::vformat(format_str, std::make_format_args(args...))) << std::endl;
	}
};

template <typename F, typename T>
void debug_log(F func, T var)
{
	#if DEBUG_LOG == 1
		func(var);
	#endif
	#if DEBUG_LOG == 0
		(void)func;
		(void)var;
	#endif
}