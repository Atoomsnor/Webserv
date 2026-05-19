#include "Logger.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::unique_ptr<std::ofstream> Logger::file = nullptr;

std::string Logger::getTime(std::string str) {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_struct = *std::localtime(&t);
	std::ostringstream ss;
	ss << std::put_time(&tm_struct, str.c_str());
	return ss.str();
}

void Logger::createLog(void)
{
	std::cout << "log created" << std::endl;
	file = std::make_unique<std::ofstream>("logs/log-" + getTime("%Y%m%d_%H%M%S"));
}
