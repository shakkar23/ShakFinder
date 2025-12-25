
#include <atomic>
#include <thread>
#include <print>

#include "Util.hpp"

#include <board.hpp>

namespace Solver {
    struct can_pc_state {
        const Game& game;
        const Queue& queue;
        std::vector<FullPiece>& path;
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
        for (size_t i = 0; i < QUEUE_SIZE && i + state.pieces_used+1 < state.queue.size(); i++) {
            game.queue.at(i) = state.queue.at(i + state.pieces_used+1);
        }

        bool T_should_be_vertical = false;
        bool T_should_be_horizontal = false;

        // shit dont work
        if(false){  // columnar parity checking
            u32 empty_cells = game.empty_cells(state.max_lines - state.cleared_lines);
            // this only works because we constrain the queue to be
            // exactly the number of pieces that fills the max_pc
            const size_t lookahead = empty_cells / 4UL;

            int t_count = 0;
            for (size_t i = 0; i < lookahead and i + state.pieces_used < state.queue.size(); i++) {
                if (state.queue.at(i + state.pieces_used) == PieceType::T) {
                    t_count++;
                }
            }

            if (game.hold.has_value() && game.hold.value() == PieceType::T)
                t_count++;

            if (game.current_piece == PieceType::T)
                t_count++;

            // check the hold piece as well as lookahead - 1 pieces in the queue
            int columnar_parity = 0;

            for (size_t i = 0; i < lookahead and i + state.pieces_used < state.queue.size(); i++)
                if (state.queue[i + state.pieces_used] == PieceType::J || state.queue[i + state.pieces_used] == PieceType::L)
                    columnar_parity++;

            for (const auto& piece : state.path) {
                if (piece.type == PieceType::J || piece.type == PieceType::L)
                    columnar_parity++;
                if (piece.type == PieceType::T && piece.r == RotationDirection::North || piece.r == RotationDirection::South)
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


        // possible piece placements
        auto reachable = game.current_piece_movegen();

        std::optional<bool> return_value;
        auto go = [&](const reachability::static_vector<Board, 4UL>&moves, bool held){
            
            for(std::size_t rot = 0; rot < moves.size(); ++rot) {
                const auto &reachable_board = moves[rot];
                if(return_value.has_value())
                    return return_value.value();
                const PieceType block_type = held ? game.hold.value_or(game.queue.front()) : game.current_piece;
                reachability::blocks::call_with_block<reachability::blocks::SRS>((char)block_type, [&]<reachability::block B>(){
                    reachability::static_for<Board::height>([&] (auto y) {
                        if(return_value.has_value())
                            return;
                        reachability::static_for<Board::width>([&] (auto x) {
                            if(return_value.has_value())
                                return;
                            if (reachable_board.template get<x,y>()) {
                                Board new_board = game.board;
                                bool valid = true;
                                reachability::static_for<B.BLOCK_PER_MINO>([&](const std::size_t mino_i) {
                                    int px = x + (B.minos[B.mino_index[rot]][mino_i][0]) + B.mino_offset[rot][0];
                                    int py = y + (B.minos[B.mino_index[rot]][mino_i][1]) + B.mino_offset[rot][1];
                                    // new_board.set(px, py);

                                    valid &= (py < (state.max_lines - state.cleared_lines));
                                });
                                if(x==6 and y==2 and block_type == PieceType::S and rot == RotationDirection::North) {
                                    //std::println("{}", valid);
                                }
                                if (!valid) {
                                    return;
                                }
                                // if we are here, the piece placement is valid

                                Game new_game = game;

                                // place the piece
                                bool held_first = new_game.place_piece(FullPiece{.type=block_type, .x=(int8_t)x, .y=(int8_t)y, .r=(int8_t)rot});

                                int lines_cleared = new_game.board.clear_full_lines();


                                // if the piece was held, we need to increment the pieces used
                                int pieces_used = state.pieces_used + 1;
                                if (held_first) {
                                    pieces_used++;
                                }

                                // if we have cleared the max lines, we pc'd
                                if (state.cleared_lines + lines_cleared == state.max_lines) {
                                    return_value = true;
                                    return;
                                }

                                // if the board is empty we have an early pc
                                if (!new_game.board.any()) {
                                    return_value = true;
                                    return;
                                }

                                // if we have used all the pieces in the queue, we can't pc
                                if (pieces_used == state.queue.size()) {
                                    return;
                                }

                                int lines_left = state.max_lines - state.cleared_lines - lines_cleared;

                                // {
                                //     // has_isolated_cell https://github.com/wirelyre/tetra-tools/blob/2342953cb424cfd5ca94fa8eefdbe5434bd5ff1c/srs-4l/src/gameplay.rs#L169
                                //     u32 not_empty = new_game.not_empty(lines_left);
                                //     u32 full = new_game.full(lines_left);
                                //     u32 bounded = new_game.bounded(lines_left);
                                //     if ((not_empty & (~full) & bounded) != 0) {
                                //         return;
                                //     }
                                // }

                                // if (new_game.board.has_imbalanced_split(lines_left))
                                //     return;

                                if (T_should_be_horizontal && block_type == PieceType::T) {
                                    if (rot == RotationDirection::North || rot == RotationDirection::South) {
                                        //return;
                                    }
                                }

                                if (T_should_be_vertical && block_type == PieceType::T) {
                                    if (rot == RotationDirection::East || rot == RotationDirection::West) {
                                        //return;
                                    }
                                }

                                state.path.emplace_back(FullPiece{.type=block_type, .x=(int8_t)x, .y=(int8_t)y, .r=(int8_t)rot});
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
                                    return_value = true;
                                    return;
                                }
                                state.path.pop_back();
                            }
                        });
                    });
                });
                if(return_value.has_value())
                    return return_value.value();
            }
            return return_value.value_or(false);
        };
        go(reachable,false);
        if(return_value.has_value())
            return return_value.value();
        auto reachable2 = game.hold_piece_movegen();
        if(game.hold.value_or(game.queue.front()) != PieceType::Empty)
            go(reachable2, true);
        return return_value.value_or(false);
    }

    // returns whether or not a pc is possible
    bool can_pc(const Board& board, const Queue& queue) {
        Game game;
        game.board = board;
        game.current_piece = queue[0];
        game.queue.fill(PieceType::Empty);
        for (size_t i = 1; i < QUEUE_SIZE && i < queue.size(); i++) {
            game.queue[i - 1] = queue[i];
        }
        const auto ppp = game.current_piece_movegen();
        const auto pppp = game.hold_piece_movegen();
        const int max_lines = 4;

        std::atomic_bool atomic_solved = false;

        int size_acc = 0;
        for (auto& pp : std::span{ppp.data, ppp.size()}) {
            size_acc += pp.popcount();
        }
        for (auto& ppppp : std::span{pppp.data, pppp.size()}) {
            size_acc += ppppp.popcount();
        }

        std::vector<std::jthread> threads(size_acc);
        std::vector<u8> return_values(size_acc);
        //std::println("{}", size_acc);

        int i = 0;
        auto go = [&] (const reachability::static_vector<Board, 4UL>& moves, bool held) {
            for(std::size_t rot = 0; rot < moves.size(); ++rot) {
                const auto &reachable_board = moves[rot];
                auto block_type = held ? game.queue[0] : game.current_piece;
                reachability::blocks::call_with_block<reachability::blocks::SRS>(block_type, [&]<reachability::block B>(){
                    reachability::static_for<Board::height>([&] (auto y) {
                        reachability::static_for<Board::width>([&] (auto x) {
                            if (reachable_board.template get<x,y>()) {
                                #ifdef MULTITHREADED
                                threads[i] = std::jthread(
                                #endif
                                [block_type=block_type,held=held,rot=rot,x=x,y=y,game, queue, &atomic_solved, max_lines, &return_value = return_values[i]]() {
                                    Game new_game = game;
                                    FullPiece p = {.type = block_type, .x = (int8_t)x, .y = (int8_t)y, .r = (int8_t)rot};
                                    bool first_hold = held;
                                    auto& new_board = new_game.board;

                                    // make sure the piece is valid
                                    bool valid = true;
                                    reachability::static_for<B.BLOCK_PER_MINO>([&](const std::size_t mino_i) {
                                        int px = x + (B.minos[B.mino_index[rot]][mino_i][0]) + B.mino_offset[rot][0];
                                        int py = y + (B.minos[B.mino_index[rot]][mino_i][1]) + B.mino_offset[rot][1];
                                        valid &= (py < (max_lines - 0)); // 0 => cleared lines
                                    });
                                    
                                    if(x==4 and y==1 and block_type == PieceType::T and rot == RotationDirection::East) {
                                        // std::println("{}", valid);
                                    }

                                    if (!valid)
                                        return;
                                    
                                    new_game.place_piece(p);
                                    auto before = new_game.board;

                                    int lines_cleared = new_game.board.clear_full_lines();
                                    if(lines_cleared != 0) {
                                        //std::println("beofre:\n{}\nafter\n{}", to_string(before), to_string(new_game.board));
                                    }
                                    
                                    if (lines_cleared == max_lines) {
                                        atomic_solved = true;
                                        return_value = true;
                                        return;
                                    }
                                    int pieces_used = 1;
                                    
                                    if (first_hold) {
                                        pieces_used++;
                                    }
                                    
                                    std::vector<FullPiece> path{};
                                    path.emplace_back(p);
                                    bool local_solved = can_pc_recurse({
                                        .game = new_game,
                                        .queue = queue,
                                        .path = path,
                                        .pieces_used = pieces_used,
                                        .cleared_lines = lines_cleared,
                                        .max_lines = max_lines }, atomic_solved);
                                        
                                        if (local_solved) {
                                            atomic_solved = true;
                                            return_value = true;
                                        }
                                        return;
                                    }
                                #ifdef MULTITHREADED
                                );
                                #else
                                ();
                                #endif
                                ++i;
                            }
                        });
                    });
                });
            }
        };

        go(ppp,false);
        go(pppp,true);

        #ifdef MULTITHREADED
        for (auto& thread : threads) {
            thread.join();
        }
        #endif

        bool ret = false;
        for (auto return_value : return_values) {
            ret |= return_value;
        }
        return ret;
    }

    struct solve_pcs_state {
        const Game& game;
        const Queue& queue;
        std::vector<FullPiece>& path;
        // solutions we found
        std::vector<std::vector<FullPiece>>& solutions;
        // pieces used thus far in the queue
        const int pieces_used;
        // lines cleared thus far
        const int cleared_lines;
        // the number of lines that we are constraining the pc to happen in
        const int max_lines;
    };

    static void solve_pcs_recurse(const solve_pcs_state& state) {}
    
    /*
    static void solve_pcs_recurse(const solve_pcs_state& state) {
        // warning with returning out of this function, it means that we are skipping every other piece placement

        Game game = state.game;

        // copy the current piece
        game.current_piece = state.queue[state.pieces_used];

        // copy the queue
        for (size_t i = 0;  i < QUEUE_SIZE   &&    i + state.pieces_used + 1 < state.queue.size(); i++) {
            game.queue[i] = state.queue[i + state.pieces_used + 1];
        }


        // columnar state to check for if the T piece should be placed in a certain orientation
        bool T_should_be_vertical = false;
        bool T_should_be_horizontal = false;

        // columnar parity checking this is mega nerd stuff
        // https://docs.google.com/document/d/1udtq235q2SdoFYwMZNu-GRYR-4dCYMkp0E8_Hw1XTyg/edit#heading=h.z6ne0og04bp5
        // this is the link to the document that explains Perfect Clear Theory
        // i dont understand much of this apparently it just works
        {  
            u32 empty_cells = game.empty_cells(state.max_lines - state.cleared_lines);
            // this only works because we constrain the queue to be
            // exactly the number of pieces that fills the max_pc
            const size_t lookahead = empty_cells / 4;

            int t_count = 0;
            for (size_t i = 0; i < lookahead; i++) {
                if (state.queue[i + state.pieces_used] == PieceType::T) {
                    t_count++;
                }
            }

            if (game.hold.has_value() && game.hold.value() == PieceType::T)
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


        // possible piece placements
        auto ppp = game.current_piece_movegen();
        auto pppp = game.hold_piece_movegen();
        
        for (auto& pp : ppp) {
            // object that I can just make that does the pop and push for recursive calls to have the correct path
            struct path_updater {
                std::vector<FullPiece>& path;

                path_updater(std::vector<FullPiece>& path, const FullPiece& pp) : path(path) {
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
                }
            }

            if (!valid) {
                continue;
            }
            Game new_game = game;

            // place the piece
            bool held_first = new_game.place_piece(pp);
            int lines_cleared = new_game.board.clear_full_lines();

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
                // bit maggic stuff, dont ask me it just works apparently
                // has_isolated_cell https://github.com/wirelyre/tetra-tools/blob/2342953cb424cfd5ca94fa8eefdbe5434bd5ff1c/srs-4l/src/gameplay.rs#L169
                u32 not_empty = new_game.board.not_empty(lines_left);
                u32 full = new_game.board.full(lines_left);
                u32 bounded = new_game.board.bounded(lines_left);
                if ((not_empty & (~full) & bounded) != 0) {
                    // creates an isolated cell, bad piece placement
                    continue;
                }
            }

            // https://github.com/wirelyre/tetra-tools/blob/2342953cb424cfd5ca94fa8eefdbe5434bd5ff1c/srs-4l/src/gameplay.rs#L240
            // this also just works
            if (new_game.board.has_imbalanced_split(lines_left))
            {
                // bad piece placement, it makes an imbalanced split
                continue;
            }

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

    }
    */


    std::vector<std::vector<FullPiece>> solve_pcs(const Board& board, const Queue& queue) {
        return {};
    }
    // returns the Moves for every PC possible
    /*
    std::vector<std::vector<FullPiece>> solve_pcs(const Board& board, const Queue& queue) {
        std::vector<std::vector<FullPiece>> solutions;

        Game game;
        game.current_piece = queue[0];

        for (size_t i = 0;  i < QUEUE_SIZE  &&  i + 1 < queue.size(); i++) {
            game.queue[i] = queue[i+1];
        }

        game.board = board;

        auto ppp = game.current_piece_movegen();
        auto pppp = game.hold_piece_movegen();
        int max_lines = 4;

        std::atomic_bool solved = false;

        std::vector<std::jthread> threads(ppp.size());
        std::vector<std::vector<std::vector<FullPiece>>> return_values(ppp.size());

        int i = 0;
        for (auto& pp : ppp) {
            threads[i] = std::jthread([game, queue, pp, &solved, max_lines, &return_value = return_values[i]]() {
                Game new_game = game;

                // make sure the piece is valid
                bool valid = true;
                for (auto& mino : pp.minos) {
                    if (pp.position.y + mino.y >= (max_lines - 0)) { // 0 => cleared lines
                        valid = false;
                    }
                }

                if (!valid) {
                    return;
                }

                bool held_first = new_game.place_piece(pp);
                int lines_cleared = new_game.board.clearLines();



                if (lines_cleared == max_lines) {
                    solved = true;
                    return_value = {{pp}};
                    return;
                }
                int pieces_used = 1;

                if (held_first) {
                    pieces_used++;
                }
                std::vector<FullPiece> path{pp};
                solve_pcs_recurse(
                    {.game = new_game,
                    .queue = queue,
                    .path = path,
                    .solutions = return_value,
                    .pieces_used = pieces_used,
                    .cleared_lines = lines_cleared,
                    .max_lines = max_lines});
                return;
            });

            ++i;
        }

        for (auto& thread : threads) {
            thread.join();
        }

        for (const auto& return_value : return_values) {
            for (const auto &solution : return_value) {
                solutions.push_back(solution);
            }
        }

        return solutions;
    }
    */
}  // namespace Solver