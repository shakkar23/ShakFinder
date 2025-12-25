#pragma once


#include <vector>

#include <search.hpp>
#include "board.hpp"
#include "block.hpp"
#include "utils.hpp"

enum PieceType : uint8_t {
    S = 'S',
    Z = 'Z',
    I = 'I',
    O = 'O',
    L = 'L',
    J = 'J',
    T = 'T',
    Empty = ' ',
};
using Queue = std::vector<PieceType>;
using u32 = uint32_t;
using u8 = uint8_t;

enum RotationDirection : uint8_t {
    North = 0,
    East = 1,
    South = 2,
    West = 3,
};

struct FullPiece {
    PieceType type;
    int8_t x = 4;
    int8_t y = 20;
    int8_t r = 0;
};
constexpr auto QUEUE_SIZE = 5;
using Board = reachability::board_t<10,24>;
struct Game {
    Board board;
    PieceType current_piece;
    std::optional<PieceType> hold;
    std::array<PieceType, QUEUE_SIZE> queue;

    auto current_piece_movegen() const {
        if (current_piece == PieceType::Empty) {
            // no moves possible, empty return
            auto ret = reachability::static_vector<Board, 4UL>{std::span<Board,0>()};
            return ret;
        }
        return reachability::search::binary_bfs<reachability::blocks::SRS, reachability::coord{4,20}>(board, current_piece);
    }
    auto hold_piece_movegen() const {
        PieceType other = hold.value_or(queue.front());
        if (other == current_piece || other == PieceType::Empty) {
            // no moves possible, empty return
            auto ret = reachability::static_vector<Board, 4UL>{std::span<Board,0>()};
            return ret;
        }
        return reachability::search::binary_bfs<reachability::blocks::SRS, reachability::coord{4,20}>(board, other);
    }
    auto empty_cells(int height) const {
        return height * 10 - board.popcount();
    }

    bool place_piece(const FullPiece& piece) {
        bool first_hold = false;
        if (piece.type != current_piece) {
            if (!hold.has_value())
            {  // shift queue
                std::shift_left(queue.begin(), queue.end(), 1);

                queue.back() = PieceType::Empty;

                first_hold = true;
            }
            hold = current_piece;
        }

        current_piece = piece.type;
        place_this_piece(piece);

        return first_hold;
    }

    private:
    void place_this_piece(FullPiece piece) {
        reachability::blocks::call_with_block<reachability::blocks::SRS>(piece.type, [&]<reachability::block B>(){
            reachability::static_for<B.BLOCK_PER_MINO>([&](const std::size_t mino_i) {
                int px = piece.x + (B.minos[B.mino_index[piece.r]][mino_i][0]) + B.mino_offset[piece.r][0];
                int py = piece.y + (B.minos[B.mino_index[piece.r]][mino_i][1]) + B.mino_offset[piece.r][1];
                board.set(px, py);
            });
        });

        current_piece = queue.front();

        std::shift_left(queue.begin(), queue.end(), 1);

        queue.back() = PieceType::Empty;
    }
};
