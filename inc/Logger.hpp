#pragma once

#include <fstream>
#include <memory>
#include <string>

class Logger {
	private:
		static std::unique_ptr<Logger> instance;
		std::unique_ptr<std::ofstream> file;
		Logger(); 
	public:
		static std::unique_ptr<Logger> &getInstance();

		Logger(Logger &cpy) = delete;
		void operator=(const Logger &cpy) = delete;
		void printLog(std::string str) const;
};