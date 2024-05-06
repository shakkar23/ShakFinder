#include "Parser.hpp"


PieceType Parser::getType(char c) {
	switch (c)
	{
	case 'T': case 't':
		return PieceType::T;
	case 'L': case 'l':
		return PieceType::L;
	case 'J': case 'j':
		return PieceType::J;
	case 'S': case 's':
		return PieceType::S;
	case 'Z': case 'z':
		return PieceType::Z;
	case 'O': case 'o':
		return PieceType::O;
	case 'I': case 'i':
		return PieceType::I;
	default:
		return PieceType::Empty;
		break;
	}
}

char Parser::getChar(PieceType p) {
	switch (p)
	{
	case PieceType::T:
		return 'T';
	case PieceType::L:
		return 'L';

	case PieceType::J:
		return 'J';

	case PieceType::S:
		return 'S';

	case PieceType::Z:
		return 'Z';

	case PieceType::O:
		return 'O';

	case PieceType::I:
		return 'I';
	default:
		return ' ';
		break;
	}

}

std::vector<PieceType> Parser::naive_parse(const std::string& str) {
	std::vector<PieceType> pieces;

	for (auto& c : str) {
		auto type = getType(c);
		if (type != PieceType::Empty)
			pieces.push_back(type);
	}

	return pieces;
}

std::vector<Queue> Parser::get_combinations(Queue queue, int num) {
	if (num > queue.size()) {
		std::cerr << "Error: p# is greater than the number of pieces in the brackets" << std::endl;
		return {};
	}
	std::vector<Queue> combinations;

	// sort the queue so its in its lexicographical order
	// aka its last permutation
	std::sort(queue.begin(), queue.end());
	do {
		Queue new_queue;
		// push back the number num to the queue
		for (int i = 0; i < num; ++i) {
			new_queue.emplace_back(queue[i]);
		}
		combinations.emplace_back(std::move(new_queue));
	} while (std::next_permutation(queue.begin(), queue.end()));

	return combinations;
}

/*
The input I,T,S,Z represents one piece queue ITSZ.
The input [SZ],O,[LJ] represents four possible piece queues SOL, SOJ, ZOL, ZOJ.
The input L,* represents seven possible piece queues LT, LI, LJ, LL, LS, LZ, LO.
The input [SZLJ]p2 represents twelve possible piece queues
SZ, SL, SJ, ZS, ZL, ZJ, LS, LZ, LJ, JS, JZ, JL.
The input *! is equivalent to [TILJSZO]p7 and represents 5040 piece queues
*/
std::vector< std::vector<PieceType>> Parser::parse(const std::string& str) {
	std::vector<Queue> pieces;
	pieces.push_back({});

	for (size_t i = 0; i < str.size(); ++i) {
		auto& c = str[i];

		// handle [ and ] pair similar to regex
		if (c == '[') {
			// find the cooresponding ]
			auto end = str.find(']', i);

			// get the substring between the brackets
			auto sub = str.substr(i + 1, end - i - 1);

			// parse the substring
			auto bag = naive_parse(sub);

			if (bag.size() == 0) {
				std::cerr << "Error: Empty brackets" << std::endl;
				return {};
			}

			// check if there is a leading p# to get the combinations of the pieces
			size_t p_loc = (str[end + 1] == 'p') ? (end + 1) : (std::string::npos);

			bool exclaimation = str[end + 1] == '!';

			if (exclaimation)
				p_loc = (end + 1);

			// if there is a p# then get the number of combinations with the given p number
			int num{};
			if (exclaimation)
				num = bag.size();
			else if (p_loc != std::string::npos)
				num = std::stoi(str.substr(p_loc + 1, str.size() - p_loc - 1));
			else
				num = 1;

			if (num > bag.size()) {
				std::cerr << "Error: p# is greater than the number of pieces in the brackets" << std::endl;
				return {};
			}

			std::vector<Queue> combs = get_combinations(bag, num);

			// make a copy of all the queues, and make a cartesian product of the queues, and the bag
			std::vector<Queue> old_pieces = std::move(pieces);

			for (const auto& comb : combs) {
				for (const auto& queue : old_pieces) {
					Queue new_queue = queue;
					new_queue.insert(new_queue.end(), comb.begin(), comb.end());
					pieces.emplace_back(std::move(new_queue));
				}
			}

			// prune out everything that isnt unique
			std::sort(pieces.begin(), pieces.end());
			pieces.erase(std::unique(pieces.begin(), pieces.end()), pieces.end());

			// set the index to the end of the brackets or p if it exists
			// this sorta sucks because it lets numbers go through the other if statements
			// but its fine cause we skip them
			i = p_loc != std::string::npos ? p_loc : end;
		}
		// seperator for human readability
		else if (c == ',') {
			continue;
		}
		else for (auto& vec : pieces)
		{
			// raw pieces being added
			// example
			// tloi[zs]p1js
			// ^^^^      ^^
			// if statement to skip numbers
			auto type = getType(c);
			if (type != PieceType::Empty)
				vec.push_back(type);
		}
	}


	return pieces;
}

std::string Parser::preprocess(const std::string input) {

	// simply find replace the '*' character with [TILJSZO]

	std::string output = input;
	std::string find = "*";
	std::string replace = "[TILJSZO]";
	size_t pos = 0;
	while ((pos = output.find(find, pos)) != std::string::npos) {
		output.replace(pos, find.length(), replace);
		pos += replace.length();
	}
	return output;
}
