#pragma once
#include "engine/Piece.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#include "Util.hpp"

namespace Parser {
	PieceType getType(char c);

	char getChar(PieceType p);

	std::vector<PieceType> naive_parse(const std::string& str);

	std::vector<Queue> get_combinations(Queue queue, int num);

	/*
	The input I,T,S,Z represents one piece queue ITSZ.
	The input [SZ],O,[LJ] represents four possible piece queues SOL, SOJ, ZOL, ZOJ.
	The input L,* represents seven possible piece queues LT, LI, LJ, LL, LS, LZ, LO.
	The input [SZLJ]p2 represents twelve possible piece queues
	SZ, SL, SJ, ZS, ZL, ZJ, LS, LZ, LJ, JS, JZ, JL.
	The input *! is equivalent to [TILJSZO]p7 and represents 5040 piece queues
	*/
	std::vector< std::vector<PieceType>> parse(const std::string& str);

	std::string preprocess(const std::string input);
};
