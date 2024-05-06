#include "ShakFinder.h"
#include "Engine/Parser.hpp"
#include "Engine/Solver.hpp"

#include <chrono>

int main(int argc, char* argv[])
{
	std::span<char*> args(argv, argc);

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	auto queues = Parser::parse(Parser::preprocess("[SZ][JL][TOI]SZTLJOL"));
	
	std::cout << "generated queues!" << std::endl;

	for (int i = 0; i < queues.size(); i++)
	{
		bool pc = Solver::can_pc(Board(), queues[i]);

		if(!pc)
		{
			std::cout << "we could not solve the pc for: " << i + 1 << std::endl;
			std::cout << "the queue is: ";
			for (PieceType& piece : queues[i])
			{
				std::cout << Parser::getChar(piece);
			}
			std::cout << std::endl << std::endl;
		}
		else 
			std::cout << "solved a thing!" << std::endl;
	}

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[mili seconds]" << std::endl;

	return 0;
}