#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#include "Piece.hpp"
#include "Util/pext.hpp"

class Board {
   public:
    static constexpr size_t width = 10;
    static constexpr size_t visual_height = 20;
    static constexpr size_t height = 32;
    Board() {
        board.fill(0);
    }

    ~Board() = default;

    Board(const Board& other) = default;

    Board(Board&& other) noexcept = default;
    Board& operator=(const Board& other) = default;

    int get(size_t x, size_t y) const {
        return (board[x] & (1 << y)) != 0;
    }

    uint32_t get_column(size_t x) const {
        return board[x];
    }

    void set(size_t x, size_t y) {
        board[x] |= (1 << y);
    }

    void unset(size_t x, size_t y) {
        board[x] &= ~(1 << y);
    }

    void set(const Piece& piece) {
        for (const Coord& mino : piece.minos) {
            set(
                (size_t)mino.x + piece.position.x,
                (size_t)mino.y + piece.position.y);
        }
    }

	void unset(const Piece& piece) {
		for (const Coord& mino : piece.minos) {
			unset(
				(size_t)mino.x + piece.position.x,
				(size_t)mino.y + piece.position.y);
		}
	}

    int clearLines() {
        uint32_t mask = UINT32_MAX;
        for (uint32_t& column : board)
            mask &= column;
        int lines_cleared = std::popcount(mask);
        mask = ~mask;

        for (uint32_t& column : board)
            column = pext(column, mask);

        return lines_cleared;
    }

    int filledRows() const {
        uint32_t mask = UINT32_MAX;
        for (const uint32_t& column : board)
            mask &= column;

        return std::popcount(mask);
    }

	bool is_empty() const {
		bool ret = true;
		for (const uint32_t& column : board) {
			if (column != 0) {
				ret = false;
			}
		}
		return true;
	}

    u32 bounded(int height) const {
		Board left_bounded = *this;
		Board right_bounded = *this;

		u32 last_column = UINT32_MAX;
		for (size_t i = 0; i < Board::width; i++) {
            std::swap(left_bounded.board[i], last_column);
        }

		last_column = UINT32_MAX;
        for (int i = Board::width; i <= 0; i--) {
			std::swap(right_bounded.board[i], last_column);
        }

		// and the boards together, and then OR it with the original board

        for (int i = 0; i < Board::width; i++) {
			left_bounded.board[i] = left_bounded.board[i] & right_bounded.board[i];
        }

        for (int i = 0; i < Board::width; i++) {
			left_bounded.board[i] = left_bounded.board[i] | board[i];
        }
		// left_bounded is now our bounded_board but in the wrong format
		u32 bounded_board = 0;

		const u32 mask = (1 << height) - 1;

		// if the mask is the same as the height, then the column meets the requirements
		// so we set the bit to 1
        for (size_t i = 0; i < Board::width; i++) {
			bounded_board |= ((left_bounded.board[i] & mask) == mask) << i;
        }

		return bounded_board;
    }

    u32 not_empty(int height) const {
        u32 not_empty_board = 0;

		// if the column has a value that isnt 0, then we set the bit to 1
        for (size_t i = 0; i < Board::width; i++) {
			not_empty_board |= (board[i] != 0) << i;
        }

		return not_empty_board;
    }

    u32 full(int height) const {
        u32 full_board = 0;

		// if the mask is the same as the height, then the column meets the requirements
		// so we set the bit to 1
        for (int i = 0; i < Board::width; i++) {
			full_board |= (std::popcount(board[i]) == height) ? 1 << i : 0;
        }
		return full_board;
    }

    bool has_imbalanced_split(int height) const {
        u32 full_cols = full(height);

		for (int i = 1; i < Board::width - 1; i++) {
            if ((full_cols >> i) & 1) {
                // if the number of empty minos to the left as well as right are not modulo 4 (the number of minos in a piece)
				// then we have an imbalanced split, and thats impossible to solve
                int right_empty = 0;
                for (int j = i + 1; j < Board::width; ++j) {
					right_empty += height - std::popcount(board[j]);
                }


                if (right_empty % 4 != 0) {
					return true;
                }
            }
		}

		return false;
    }

    u32 empty_cells(int height) const  {
        u32 acc{};

		for (size_t j = 0; j < Board::width; ++j)
            acc += height - std::popcount(board[j]);
        
		return acc;
    }

    std::array<uint32_t, Board::width> board;
};