#include "Solver.hpp"

#include <atomic>
#include <thread>

#include "Game.hpp"

namespace Solver {

struct can_pc_state {
    const Game& game;
    const Queue& queue;
    std::vector<Piece>& path;
    // pieces used thus far in the queue
    const int pieces_used;
    // lines cleared thus far
    const int cleared_lines;
    // the number of lines that we are constraining the pc to happen in
    const int max_lines;
};

static bool can_pc_recurse(const can_pc_state& state, std::atomic_bool& solved) {
    // warning with returning out of this function, it means that we are skipping every other piece placement
    if (solved) {
        return true;
    }

    Game game = state.game;

    // copy the queue
    for (size_t i = 0; i < QUEUE_SIZE && i + state.pieces_used < state.queue.size(); i++) {
        game.queue[i] = state.queue[i + state.pieces_used];
    }

    // possible piece placements
    std::vector<Piece> ppp = game.get_possible_piece_placements();

    bool T_should_be_vertical = false;
    bool T_should_be_horizontal = false;

    {  // columnar parity checking
        u32 empty_cells = game.board.empty_cells(state.max_lines - state.cleared_lines);
        // this only works because we constrain the queue to be
        // exactly the number of pieces that fills the max_pc
        size_t lookahead = empty_cells / 4;

        int t_count = 0;
        for (size_t i = 0; i < lookahead; i++) {
            if (state.queue[i + state.pieces_used] == PieceType::T) {
                t_count++;
            }
        }

        if (game.hold.has_value() && game.hold.value().type == PieceType::T)
            t_count++;

        if (game.current_piece.type == PieceType::T)
            t_count++;

        // check the hold piece as well as lookahead - 1 pieces in the queue
        int columnar_parity = 0;

        for (size_t i = 0; i < lookahead - 1; i++)
            if (state.queue[i + state.pieces_used] == PieceType::J || state.queue[i + state.pieces_used] == PieceType::L)
                columnar_parity++;

        for (const auto& piece : state.path) {
            if (piece.type == PieceType::J || piece.type == PieceType::L)
                columnar_parity++;
            if (piece.type == PieceType::T && piece.rotation == RotationDirection::North || piece.rotation == RotationDirection::South)
                columnar_parity++;
        }

        if (t_count == 0) {
            // count the number of J and L pieces used and left in the queue
            // as well as the number of vertical Ts in the path
            // if the sum of these is odd, we have an invalid columnar parity
            if (columnar_parity % 2 == 1)
                return false;
        } else if (t_count == 1) {
            // if we have a single T piece, we need to make sure this T piece is vertical to account for the columnar parity

            if (columnar_parity % 2 == 1)
                T_should_be_vertical = true;
            else
                T_should_be_horizontal = true;
        }

    }  // columnar parity

    for (auto& pp : ppp) {
        // prune out every invalid piece placement
        // aka the ones that go against the max_lines constraint

        bool valid = true;
        for (auto& mino : pp.minos) {
            if (pp.position.y + mino.y >= (state.max_lines - state.cleared_lines)) {
                valid = false;
                break;
            }
        }

        if (!valid) {
            continue;
        }
        Game new_game = game;

        // place the piece
        bool held_first = new_game.place_piece(pp);
        int lines_cleared = new_game.board.clearLines();

        // if the piece was held, we need to increment the pieces used
        int pieces_used = state.pieces_used + 1;
        if (held_first) {
            pieces_used++;
        }

        // if we have cleared the max lines, we pc'd
        if (state.cleared_lines + lines_cleared == state.max_lines) {
            return true;
        }

        // if the board is empty we have an early pc
        if (new_game.board.is_empty()) {
            return true;
        }

        // if we have used all the pieces in the queue, we can't pc
        if (pieces_used == state.queue.size()) {
            continue;
        }

        int lines_left = state.max_lines - state.cleared_lines - lines_cleared;

        {
            // has_isolated_cell https://github.com/wirelyre/tetra-tools/blob/2342953cb424cfd5ca94fa8eefdbe5434bd5ff1c/srs-4l/src/gameplay.rs#L169
            u32 not_empty = new_game.board.not_empty(lines_left);
            u32 full = new_game.board.full(lines_left);
            u32 bounded = new_game.board.bounded(lines_left);
            if ((not_empty & (~full) & bounded) != 0) {
                continue;
            }
        }

        if (new_game.board.has_imbalanced_split(lines_left))
            continue;

        if (T_should_be_horizontal && pp.type == PieceType::T) {
            if (pp.rotation == RotationDirection::North || pp.rotation == RotationDirection::South) {
                continue;
            }
        }

        if (T_should_be_vertical && pp.type == PieceType::T) {
            if (pp.rotation == RotationDirection::East || pp.rotation == RotationDirection::West) {
                continue;
            }
        }

        state.path.emplace_back(pp);
        // we havent pc'd yet and we have more pieces to use
        // recurse
        if (can_pc_recurse(
                {.game = new_game,
                 .queue = state.queue,
                 .path = state.path,
                 .pieces_used = pieces_used,
                 .cleared_lines = state.cleared_lines + lines_cleared,
                 .max_lines = state.max_lines},
                solved)) {
            return true;
        }
        state.path.pop_back();
    }
    return false;
}

// returns whether or not a pc is possible
bool can_pc(const Board& board, const Queue& queue) {
    Game game;
    game.current_piece = queue[0];

    for (size_t i = 1; i < QUEUE_SIZE && i < queue.size(); i++) {
        game.queue[i - 1] = queue[i + 1];
    }
    std::vector<Piece> ppp = game.get_possible_piece_placements();

    int max_lines = 4;

    std::atomic_bool solved = false;

    std::vector<std::jthread> threads(ppp.size());
    std::vector<u8> return_values(ppp.size());

    int i = 0;
    for (auto& pp : ppp) {
        threads[i] = std::jthread([game, queue, pp, &solved, max_lines, &return_value = return_values[i]]() {
            Game new_game = game;
            Piece p = pp;
            bool held_first = new_game.place_piece(p);
            int lines_cleared = new_game.board.clearLines();

            if (lines_cleared == max_lines) {
                solved = true;
                return_value = true;
                return;
            }
            int pieces_used = 0;

            if (held_first) {
                pieces_used++;
            }
            std::vector<Piece> path{};
            bool solved2 = can_pc_recurse({.game = new_game, .queue = queue, .path = path, .pieces_used = pieces_used, .cleared_lines = lines_cleared, .max_lines = max_lines}, solved);
            if (solved2) {
                solved = true;
                return_value = true;
            }
            return;
        });

        ++i;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (auto return_value : return_values) {
        if (return_value) {
            return true;
        }
    }

    return false;
}

struct solve_pcs_state {
    const Game& game;
    const Queue& queue;
    std::vector<Piece>& path;
    // solutions we found
    std::vector<std::vector<Piece>>& solutions;
    // pieces used thus far in the queue
    const int pieces_used;
    // lines cleared thus far
    const int cleared_lines;
    // the number of lines that we are constraining the pc to happen in
    const int max_lines;
};

void solve_pcs_recurse(const solve_pcs_state& state) {
    // warning with returning out of this function, it means that we are skipping every other piece placement

    Game game = state.game;

    // copy the queue
    for (size_t i = 0; i < QUEUE_SIZE && i + state.pieces_used < state.queue.size(); i++) {
        game.queue[i] = state.queue[i + state.pieces_used];
    }

    // possible piece placements
    std::vector<Piece> ppp = game.get_possible_piece_placements();

    bool T_should_be_vertical = false;
    bool T_should_be_horizontal = false;

    {  // columnar parity checking
        u32 empty_cells = game.board.empty_cells(state.max_lines - state.cleared_lines);
        // this only works because we constrain the queue to be
        // exactly the number of pieces that fills the max_pc
        size_t lookahead = empty_cells / 4;

        int t_count = 0;
        for (size_t i = 0; i < lookahead; i++) {
            if (state.queue[i + state.pieces_used] == PieceType::T) {
                t_count++;
            }
        }

        if (game.hold.has_value() && game.hold.value().type == PieceType::T)
            t_count++;

        if (game.current_piece.type == PieceType::T)
            t_count++;

        // check the hold piece as well as lookahead - 1 pieces in the queue
        int columnar_parity = 0;

        for (size_t i = 0; i < lookahead - 1; i++)
            if (state.queue[i + state.pieces_used] == PieceType::J || state.queue[i + state.pieces_used] == PieceType::L)
                columnar_parity++;

        for (const auto& piece : state.path) {
            if (piece.type == PieceType::J || piece.type == PieceType::L)
                columnar_parity++;
            if (piece.type == PieceType::T && piece.rotation == RotationDirection::North || piece.rotation == RotationDirection::South)
                columnar_parity++;
        }

        if (t_count == 0) {
            // count the number of J and L pieces used and left in the queue
            // as well as the number of vertical Ts in the path
            // if the sum of these is odd, we have an invalid columnar parity
            // the pc isnt possible here
            if (columnar_parity % 2 == 1)
                return;
        } else if (t_count == 1) {
            // if we have a single T piece, we need to make sure this T piece is vertical to account for the columnar parity

            if (columnar_parity % 2 == 1)
                T_should_be_vertical = true;
            else
                T_should_be_horizontal = true;
        }

    }  // columnar parity

    for (auto& pp : ppp) {
        // make object that does pop back on destruction
        struct path_updater {
            std::vector<Piece>& path;
            path_updater(std::vector<Piece>& path, const Piece& pp) : path(path) {
                path.emplace_back(pp);
            }
            ~path_updater() {
                path.pop_back();
            }
        } updater(state.path, pp);

        // prune out every invalid piece placement
        // aka the ones that go against the max_lines constraint

        bool valid = true;
        for (auto& mino : pp.minos) {
            if (pp.position.y + mino.y >= (state.max_lines - state.cleared_lines)) {
                valid = false;
                break;
            }
        }

        if (!valid) {
            continue;
        }
        Game new_game = game;

        // place the piece
        bool held_first = new_game.place_piece(pp);
        int lines_cleared = new_game.board.clearLines();

        // if the piece was held, we need to increment the pieces used
        int pieces_used = state.pieces_used + 1;
        if (held_first) {
            pieces_used++;
        }

        // if we have cleared the max lines, we pc'd
        if (state.cleared_lines + lines_cleared == state.max_lines) {
            // save the path to the solutions
            state.solutions.push_back(state.path);

            continue;
        }

        // if the board is empty we have an early pc
        if (new_game.board.is_empty()) {
            state.solutions.push_back(state.path);
            continue;
        }

        // if we have used all the pieces in the queue, we can't pc because the board isnt empty for some reason
        // definitely need to check this
        if (pieces_used == state.queue.size()) [[unlikely]] {
            // this might actually be wrong, but this probably means that
            // the queue isnt large enough if we got here and didnt PC
            // there should be a check before this function is ever called in order to
            // ensure that the queue is large enough
            throw;
            continue;
        }

        int lines_left = state.max_lines - state.cleared_lines - lines_cleared;

        {
            // has_isolated_cell https://github.com/wirelyre/tetra-tools/blob/2342953cb424cfd5ca94fa8eefdbe5434bd5ff1c/srs-4l/src/gameplay.rs#L169
            u32 not_empty = new_game.board.not_empty(lines_left);
            u32 full = new_game.board.full(lines_left);
            u32 bounded = new_game.board.bounded(lines_left);
            if ((not_empty & (~full) & bounded) != 0) {
                // creates an isolated cell, bad piece placement
                continue;
            }
        }

        if (new_game.board.has_imbalanced_split(lines_left))
            // bad piece placement, it makes an imbalanced split
            continue;

        if (T_should_be_horizontal && pp.type == PieceType::T) {
            if (pp.rotation == RotationDirection::North || pp.rotation == RotationDirection::South) {
                // the piece is vertical, but it should be horizontal
                continue;
            }
        }

        if (T_should_be_vertical && pp.type == PieceType::T) {
            if (pp.rotation == RotationDirection::East || pp.rotation == RotationDirection::West) {
                // the piece is horizontal, but it should be vertical
                continue;
            }
        }

        // we havent pc'd yet and we have more pieces to use
        // recurse
        solve_pcs_recurse(
            {.game = new_game,
             .queue = state.queue,
             .path = state.path,
             .solutions = state.solutions,
             .pieces_used = pieces_used,
             .cleared_lines = state.cleared_lines + lines_cleared,
             .max_lines = state.max_lines});
    }
    return;
}
// returns the Moves for every PC possible
std::vector<std::vector<Piece>> solve_pcs(const Board& board, const Queue& queue) {
    std::vector<std::vector<Piece>> solutions;

    Game game;
    game.current_piece = queue[0];

    for (size_t i = 1; i < QUEUE_SIZE && i < queue.size(); i++) {
        game.queue[i - 1] = queue[i + 1];
    }
    std::vector<Piece> ppp = game.get_possible_piece_placements();

    int max_lines = 4;

    std::atomic_bool solved = false;

    std::vector<std::jthread> threads(ppp.size());
    std::vector<std::vector<std::vector<Piece>>> return_values(ppp.size());

    int i = 0;
    for (auto& pp : ppp) {
        [game, queue, pp, &solved, max_lines, &return_value = return_values[i]]() {
            Game new_game = game;
            Piece p = pp;
            bool held_first = new_game.place_piece(p);
            int lines_cleared = new_game.board.clearLines();

            if (lines_cleared == max_lines) {
                solved = true;
                return_value = {{pp}};
                return;
            }
            int pieces_used = 0;

            if (held_first) {
                pieces_used++;
            }
            std::vector<Piece> path{pp};
            solve_pcs_recurse(
                {.game = new_game,
                 .queue = queue,
                 .path = path,
                 .solutions = return_value,
                 .pieces_used = pieces_used,
                 .cleared_lines = lines_cleared,
                 .max_lines = max_lines});
            return;
        }();

        ++i;
    }

    for (auto& thread : threads) {
        // thread.join();
    }

    for (auto return_value : return_values) {
        for (auto solution : return_value) {
            solutions.push_back(solution);
        }
    }

    return solutions;
}

}  // namespace Solver