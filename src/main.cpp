#include "Logger.hpp"

void subf2(int a, std::string b)
{
	std::unique_ptr<Logger>& lp = Logger::getInstance();
	lp->printLog("Skill Rating: {}, Match Making Rating: {}", a, b);
}

int main(void)
{
	subf2(5, "69");
	subf2(-5, "Bronze");
	subf2(696969, "GM");
}