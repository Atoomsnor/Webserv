#pragma once

#include <memory>

class Logger {
	private:
		static std::unique_ptr<Logger> instance;

		Logger(); 
	public:
		static std::unique_ptr<Logger> &getInstance();

		Logger(Logger &cpy) = delete;
		void operator=(const Logger &cpy) = delete;
		void printMemory(void) const;
};