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
	file = std::make_unique<std::ofstream>("logs/" + getTime("%Y%m%d_%H%M%S") + ".log");
	if (file == NULL || !file->is_open())
		throw std::runtime_error("epoll_wait() error"); // Don't know if caught in a try statement, but could fail
	std::cout << "log created" << std::endl;
}
