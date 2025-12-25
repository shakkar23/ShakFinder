#pragma once

#include <vector>

#include "Util.hpp"

namespace Solver {
    // returns whether or not a pc is possible
    bool can_pc(const Board& board, const Queue& queue);

    // returns the Moves for every PC possible
    std::vector<std::vector<FullPiece>> solve_pcs(const Board& board, const Queue& queue);
};