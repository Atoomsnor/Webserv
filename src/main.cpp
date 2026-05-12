#include "Logger.hpp"

void subf2(void)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	lp->printMemory();
}

int main(void)
{
	subf2();
	subf2();
	subf2();
}