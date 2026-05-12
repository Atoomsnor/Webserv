#include "Logger.hpp"
#include <fstream>
#include <iostream>
#include <chrono>

std::unique_ptr<Logger> Logger::instance = nullptr;

Logger::Logger()
{
	std::cout << "Logger instance created." << std::endl;
	file = std::make_unique<std::ofstream>("log");
}

std::unique_ptr<Logger> &Logger::getInstance(void)
{
	if (instance == nullptr)
		instance.reset(new Logger());
	return (instance);
}

void Logger::printLog(std::string str) const
{
	*file << &instance << ": " << str << std::endl;
}