#pragma once
#include <vector>
#include <cstddef>

#include "Constants.hpp"

class Piece {
   public:
    constexpr Piece(PieceType type) {
        this->type = type;
        rotation = RotationDirection::North;
        position = {10 / 2 - 1, 20 - 2};
        minos = piece_definitions[static_cast<size_t>(type)];
        spin = Spin::null;
    }

	constexpr Piece(PieceType type, Coord position, RotationDirection rotation) {
		this->type = type;
		this->position = position;
		this->rotation = rotation;
		minos = piece_definitions[static_cast<size_t>(type)];
		// rotate the piece to the correct orientation
		for (int i = 0; i < static_cast<int>(rotation); i++) {
			calculate_rotate(Right);
		}
		spin = Spin::null;
	}

    Piece() = delete;
    Piece(const Piece& other) = default;
    Piece(Piece&& other) noexcept = default;
    ~Piece() = default;
    Piece& operator=(const Piece& other) = default;

    void calculate_rotate(TurnDirection direction);
    void rotate(TurnDirection direction);
    uint32_t hash() const;
    uint32_t compact_hash() const;

    std::array<Coord, 4> minos;
    Coord position;
    RotationDirection rotation;
    PieceType type;
    Spin spin;
};