#include "ShakFinder.h"

#include <chrono>

#include "Engine/Parser.hpp"
#include "Engine/Solver.hpp"
#include "Engine/Fumen.hpp"

int main(int argc, char* argv[]) {
    std::span<char*> args(argv, argc);

    auto fumen = Fumen::parse("v115@vhAAgH");
    if (!fumen.has_value())
    {
		std::cout << "could not parse fumen" << std::endl;
		return 1;
    }

    Board board = Fumen::to_board(fumen.value().pages[0].field);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    auto queues = Parser::parse(Parser::preprocess("*!"));

    std::cout << "generated queues!" << std::endl;

    for (int i = 0; i < queues.size(); i++) {
        auto pcs = Solver::solve_pcs(board, queues[i]);

        if (pcs.size() == 0) {
            std::cout << "we could not solve the pc for: " << (i + 1) << std::endl;
            std::cout << "the queue is: ";
            for (PieceType& piece : queues[i]) {
                std::cout << Parser::getChar(piece);
            }
            std::cout << std::endl
                << std::endl;
        }
        else {
            std::cout << "solved a thing!" << std::endl;
            std::cout << "the amount of PCs found is " << pcs.size() << std::endl;
            for (const auto& path : pcs) {
                if (path.size() != 10) {
                    std::cout << "the path is " << path.size() << " long: ";

                    for (const auto& piece : path) {
                        std::cout << Parser::getChar(piece.type);
                    }
                    std::cout << std::endl;
                }
            }
        }

        std::cout << "number of pcs: " << pcs.size() << std::endl;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[mili seconds]" << std::endl;
    return 0;
}