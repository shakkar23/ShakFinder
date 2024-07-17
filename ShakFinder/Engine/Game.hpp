#pragma once
#include <array>
#include <cmath>
#include <optional>
#include <ranges>
#include <vector>
#include <variant>

#include "Board.hpp"
#include "Piece.hpp"
#include "Constants.hpp"
#include "GameRules/tetrio.hpp"
#include "GameRules/jstris_score.hpp"

constexpr size_t QUEUE_SIZE = 6;

class Game {
   public:
    Game() : current_piece(PieceType::Empty) {
		queue.fill(PieceType::Empty);
    }
    Game& operator=(const Game& other) {
        if (this != &other) {
            board = other.board;
            current_piece = other.current_piece;
            hold = other.hold;
            queue = other.queue;
            garbage_meter = other.garbage_meter;
			//stats = other.stats;
        }
        return *this;
    }
    ~Game() {}

    void place_piece();

    // returns if the first held was triggered
    bool place_piece(const Piece& piece);

    void do_hold();

    bool collides(const Board& board, const Piece& piece) const;

    void rotate(Piece& piece, TurnDirection dir) const;

    void shift(Piece& piece, int dir) const;

    void sonic_drop(const Board& board, Piece& piece) const;

    void add_garbage(int lines, int location);
private:
    int damage_sent(int linesCleared, Spin Spin, bool pc);
    public:
    void process_movement(Piece& piece, Movement movement) const;

    int get_garbage_height() const;

    bool is_convex() const;

    std::vector<Piece> movegen(PieceType piece_type) const;

    std::vector<Piece> movegen_fast(PieceType piece_type) const;

    std::vector<Piece> get_possible_piece_placements() const;

    Board board;
    Piece current_piece;
    std::optional<Piece> hold;
    std::array<PieceType, QUEUE_SIZE> queue;
    int garbage_meter = 0;

    //std::variant<Points> stats;

};