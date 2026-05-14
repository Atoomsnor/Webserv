#include "Logger.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::unique_ptr<Logger> Logger::instance = nullptr;

std::string Logger::get_time(void) const {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_struct = *std::localtime(&t);
	std::ostringstream ss;
	ss << std::put_time(&tm_struct, "%Y%m%d_%H%M%S");
	return ss.str();
}

Logger::Logger()
{
	std::cout << "Logger instance created." << std::endl;
	file = std::make_unique<std::ofstream>("logs/log-" + get_time());
}

std::unique_ptr<Logger> &Logger::getInstance(void)
{
	if (instance == nullptr)
		instance = std::unique_ptr<Logger>(new Logger());
	return (instance);
}