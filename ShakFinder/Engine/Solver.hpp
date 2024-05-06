#pragma once

#include "Piece.hpp"
#include "Board.hpp"


namespace Solver {

	// returns whether or not a pc is possible
	bool can_pc(const Board& board, const Queue& queue);


	// returns the Moves for every PC possible
	std::vector<Piece> solve_pcs(const Board& board, const Queue& queue);
};