#include "ShakFinder.h"

#include <chrono>
#include <cstring>

#include "Solver/Parser.hpp"
#include "Solver/Solver.hpp"
#include "Solver/Fumen.hpp"

int main(int argc,const char* argv[]) {
    std::span<const char*> args(argv, argc);
	
    std::vector vargs = std::vector(args.begin(), args.end());

    if(vargs.size() < 4)
        vargs = {
            "ShakFinder",
            //"v115@vhAAgH", // empty 
            "v115@9gD8DeF8CeG8BeH8CeC8JeAgH", // pco opener
            "percents",
            "zstt"
        };

    if (vargs.size() < 4) {
        std::cout << "Usage: ./" << args[0] << " <fumen> <paths|percents> <queue>" << std::endl;
        return 1;
    }

    auto fumen = Fumen::parse(vargs[1]);
    if (!fumen.has_value())
    {
        std::cout << "could not parse fumen" << std::endl;
        return 1;
    }
    //fumen.value().pages[0].print_field();

    auto board = Fumen::to_board(fumen.value().pages[0].field);

    auto begin = std::chrono::steady_clock::now();

    auto queues = Parser::parse(Parser::preprocess(vargs[3]));
    std::cout << "generated queues!" << std::endl;
    
    if (strcmp(vargs[2], "percents") == 0) {
		size_t total_solved = 0;
        for (int i = 0; i < queues.size(); i++) {
			bool solved = Solver::can_pc(board, queues[i]);
        
			if (!solved) {
				std::cout << "we could not solve the pc for: " << (i + 1) << std::endl;
				std::cout << "the queue is: ";
				for (const PieceType& piece : queues[i]) {
					std::cout << Parser::getChar(piece);
				}
				std::cout << std::endl
					<< std::endl;
			}
			else {
                total_solved++;
				std::cout << "solved a thing!" << std::endl;
				std::cout << "the queue is: ";
				for (const PieceType& piece : queues[i]) {
					std::cout << Parser::getChar(piece);
				}

				std::cout << std::endl
					<< std::endl;
			}
        }
    
		std::cout << "solved/total: " << total_solved  << "/" << queues.size() << std::endl;
		std::cout << "percentage: " << ((float)total_solved / queues.size()) * 100.0f << "%" << std::endl;
    }
    else if(strcmp(vargs[2], "paths") == 0) {
        size_t total_solved = 0;
        for (int i = 0; i < queues.size(); i++) {
            auto pcs = Solver::solve_pcs(board, queues[i]);

            if (pcs.size() == 0) {
                std::cout << "we could not solve the pc for: " << (i + 1) << std::endl;
                //std::cout << "the queue is: ";
                for (const PieceType& piece : queues[i]) {
                    std::cout << Parser::getChar(piece);
                }
                std::cout << std::endl << std::endl;
            }
            else {
                std::cout << "solved a thing!" << std::endl;
                std::cout << "the amount of PCs found is " << pcs.size() << std::endl;
                for (const auto& path : pcs) {
                    if (path.size() != 10) {
                        std::cout << "the path is " << path.size() << " long: " << std::endl;

                        for (const auto& piece : path) {
                            std::cout << "\t" << Parser::getChar(piece.type) << ": x=" << int(piece.x) << " y=" << int(piece.y) << std::endl;
                        }
                        std::cout << std::endl;
                    }
                }
                std::cout << "number of pcs: " << pcs.size() << std::endl;
                
                total_solved++;
            }

        }
        std::cout << "solved/total: " << total_solved  << "/" << queues.size() << std::endl;
    }
    	else {
		std::cout << "Usage: ./" << args[0] << " <fumen> <paths|percents> <queue>" << std::endl;
		return 1;
	}

    auto end = std::chrono::steady_clock::now();

    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1e9 << "[seconds]" << std::endl;

    return 0;
}