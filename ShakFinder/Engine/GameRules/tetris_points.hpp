#pragma once

#include "../Constants.hpp"


struct PieceStats {
    // per piece basis
    const int linesCleared;
    const Spin spin;
    const bool pc;
};


class Points {
public:
    virtual int calculate(PieceStats stats) = 0;
    virtual ~Points() = default;
};
