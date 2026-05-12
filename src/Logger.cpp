#include "Logger.hpp"
#include <iostream>

std::unique_ptr<Logger> Logger::instance = nullptr;

Logger::Logger()
{
	std::cout << "Logger instance created." << std::endl;
}

std::unique_ptr<Logger> &Logger::getInstance(void)
{
	if (instance == nullptr)
		instance = std::unique_ptr<Logger>(new Logger());
	return (instance);
}

void Logger::printMemory(void) const
{
	std::cout << &instance << std::endl;
}