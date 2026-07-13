#include "exe.h"
#include "lib.h"

#include <iostream>

namespace DemoExe
{
	std::string getHelloString()
	{
		return "Hello from demo executable!\n" + DemoLib::getDemoMessage();
	}
}

int main(int argc, char* argv[])
{
	std::cout << DemoExe::getHelloString();
	std::cin.sync();
	std::cin.clear();
	std::cin.get();
}