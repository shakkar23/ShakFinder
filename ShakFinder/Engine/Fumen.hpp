#pragma once
#include "Board.hpp"

#include <vector>
#include <array>
#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <ranges>


namespace Fumen {

	const constexpr std::array<u8, 64> BASE64_CHARS = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
			'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
			'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
			'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
			'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', '+', '/'
	};

	static std::optional<u8> from_base64(char c) {
		if ('A' <= c && c <= 'Z') {
			return c - 'A';
		}
		else if ('a' <= c && c <= 'z') {
			return c - 'a' + 26;
		}
		else if ('0' <= c && c <= '9') {
			return c - '0' + 52;
		}
		else if (c == '+') {
			return 62;
		}
		else if (c == '/') {
			return 63;
		}
		else {
			return std::nullopt;
		}
	}

	enum CellColor {
		Empty = 0,
		I = 1,
		L = 2,
		O = 3,
		Z = 4,
		T = 5,
		J = 6,
		S = 7,
		Gray = 8,
	};

	using FumenBoard = std::array<std::array<CellColor, 10>, 23>;
	using DeltaBoard = std::array<std::array<u8, 10>, 24>;

	struct Page {
		std::optional<Piece> piece;
		FumenBoard field;
		std::array<CellColor, 10> garbage_row;
		bool rise = false;
		bool mirror = false;
		bool lock = false;
		// supposed to be an optional but empty will be null

		std::string comment;

	};

	static inline std::string js_escape(std::wstring& str) {
		const std::array<u8, 16> HEX_DIGITS = {
			'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
		};

		std::string result;

		for (auto c : str) {
			auto iter = std::find(HEX_DIGITS.begin(), HEX_DIGITS.end(), c);
			if (iter != HEX_DIGITS.end()) {
				result.push_back(c);
			}
			else if (0 <= c && c <= 0xff) {
				result.push_back('%');
				result.push_back(HEX_DIGITS[c >> 4 & 0xF]);
				result.push_back(HEX_DIGITS[c >> 0 & 0xF]);
			}
			else {
				// decode from utf16 to utf8
				result.push_back('%');
				result.push_back('u');
				result.push_back(HEX_DIGITS[c >> 12 & 0xF]);
				result.push_back(HEX_DIGITS[c >> 8 & 0xF]);
				result.push_back(HEX_DIGITS[c >> 4 & 0xF]);
				result.push_back(HEX_DIGITS[c >> 0 & 0xF]);
			}
		}

		return result;
	}

	struct Fumen {
		std::vector<Page> pages;
		bool guideline = false;

		constexpr Page& add_page() {
			if (pages.size() > 0) {
				pages.push_back(pages.back());
			}
			else {
				pages.push_back(Page());
			}
			return pages.back();
		};
	};

	// pretty much taken directly from https://github.com/MinusKelvin/fumen-rs/blob/3c361f2df8cc2d40bff74e51ab38ffe8ee327cb4/src/lib.rs#L172
	static constexpr std::optional<Fumen> parse(const std::string str) {
			try {
				// if the beginning of the string isnt v115@, return an empty board
				if (str.substr(0, 5) != "v115@") {
					return std::nullopt;
				}
				else {
					// views are lazy, so i need to make a copy of the string
					std::string sub_str = str.substr(5);

					auto vec = [&sub_str]() {

						auto range = std::ranges::filter_view(sub_str, [](char c) { return c != '?'; }) |
							std::views::transform([](char c) -> std::optional<u8> { return from_base64(c); });

						std::vector<std::optional<u8>> vec;
						
						for (auto c : range) {
							vec.push_back(c);
						}

						return vec;
					}();

					// make generator that just gets the next value
					auto iter_next = [&vec] () mutable -> std::optional<u8> {
						auto& val = *vec.begin();
						// iterate iter
						vec.erase(vec.begin());
						return val;
					};

					Fumen fumen;
					int empty_fields = 0;

					while (!vec.empty()) {
						Page& page = fumen.add_page();
						if (empty_fields == 0) {
							// decode field spec
							DeltaBoard delta{};
							int x = 0;
							int y = 0;

							while (y != 24) {
								const int number = *iter_next() + 64 * *iter_next();
								const int value = number / 240;
								const int repeats = number % 240 + 1;

								for (int i = 0; i < repeats; i++) {
									if (y == 24)
										return std::nullopt;

									delta.at(y).at(x) = value;
									
									x += 1;

									if (x == 10) {
										y += 1;
										x = 0;
									}
								}
							}

							bool delta_is_empty = true;

							for (size_t y = 0; y < 24; ++y) {
								for (size_t x = 0; x < 10; ++x) {
									if (delta.at(y).at(x) != 0) {
										delta_is_empty = false;
										break;
									}
								}
							}

							if (delta_is_empty) {
								++empty_fields;
							}

							for (size_t y = 0; y < 23; ++y) {
								for (size_t x = 0; x < 10; ++x) {
									int value = delta.at(y).at(x) + page.field.at(22 - y).at(x) - 8;
									page.field.at(22 - y).at(x) = static_cast<CellColor>(value);
								}
							}

							// decode garbage row
							for (size_t x = 0; x < 10; ++x) {
								int value = delta.at(23).at(x) + size_t(page.garbage_row.at(x)) - 8;
								page.garbage_row.at(x) = static_cast<CellColor>(value);
							}

						}
						else {
							--empty_fields;
						}

						// decode page data

						int number = (int)iter_next().value() + 64 * (int)iter_next().value() + (int)iter_next().value() * 64 * 64;
						const int piece_type = number % 8;
						const int piece_rotation = number / 8 % 4;
						const unsigned int piece_pos = number / 32 % 240;

						if (piece_type == 0) {
							page.piece = Piece(PieceType::Empty);
						}
						else {
							PieceType type;
							switch (piece_type) {
							case 1:
								type = PieceType::I;
								break;
							case 2:
								type = PieceType::L;
								break;
							case 3:
								type = PieceType::O;
								break;
							case 4:
								type = PieceType::Z;
								break;
							case 5:
								type = PieceType::T;
								break;
							case 6:
								type = PieceType::J;
								break;
							case 7:
								type = PieceType::S;
								break;
							}

							RotationDirection rot;
							switch (piece_rotation) {
							case 0:
								rot = RotationDirection::North;
								break;
							case 1:
								rot = RotationDirection::East;
								break;
							case 2:
								rot = RotationDirection::South;
								break;
							case 3:
								rot = RotationDirection::West;
								break;
							}

							s8 x = s8(piece_pos % 10);
							s8 y = s8(piece_pos / 10);

							// convert fumen centers to SRS true rotation centers

							// convert x first
							if (type == PieceType::S) {
								if (rot == RotationDirection::East) {
									x -= 1;
								}
							}
							else if (type == PieceType::Z) {
								if (rot == RotationDirection::West) {
									x += 1;
								}
							}
							else if (type == PieceType::O) {
								if (rot == RotationDirection::West || rot == RotationDirection::South) {
									x += 1;
								}
							}
							else if (type == PieceType::I) {
								if (rot == RotationDirection::South) {
									x += 1;
								}
							}

							// convert y
							// types to check: S, Z, O, I
							if (type == PieceType::S) {
								if (rot == RotationDirection::North) {
									y -= 1;
								}
							}
							else if (type == PieceType::Z) {
								if (rot == RotationDirection::North) {
									y -= 1;
								}
							}
							else if (type == PieceType::O) {
								if (rot == RotationDirection::North || rot == RotationDirection::West) {
									y -= 1;
								}
							}
							else if (type == PieceType::I) {
								if (rot == RotationDirection::West) {
									y -= 1;
								}
							}

							page.piece = Piece(type, { x,y }, rot);
						}

						int flags = number / 32 / 240;

						page.rise = (flags & 0b00001) != 0;
						page.mirror = (flags & 0b00010) != 0;
						bool guideline = (flags & 0b00100) != 0;
						bool comment = (flags & 0b01000) != 0;
						page.lock = (flags & 0b10000) != 0;

						if (comment) {
							size_t length = (size_t)iter_next().value() + (size_t)iter_next().value() * 64;
							std::wstring escaped;
							while (length > 0) {
								int comment_number = (int)iter_next().value() + (int)iter_next().value() * 64 + int(iter_next().value()) * 64 * 64 +
									int(iter_next().value()) * 64 * 64 * 64 + int(iter_next().value()) * 64 * 64 * 64 * 64;
								for (int i = 0; i < std::min(length, 4ull); ++i) {
									escaped.push_back(number % 96 + 0x20);
									length -= 1;
									number /= 96;
								}
							}
							page.comment = js_escape(escaped);
						}

						if (fumen.pages.size() == 1) {
							fumen.guideline = guideline;
						}

						return fumen;
					}
				}
			}
			catch (...) {
				return std::nullopt;
			}
		}

		Board to_board(const FumenBoard& fumen_board) {
			Board board;
			
			for (size_t y = 0; y < 23; ++y)
				for (size_t x = 0; x < 10; ++x)
					if (fumen_board.at(y).at(x) != CellColor::Empty)
						board.set(x,y);
			
			return board;
		}
}; 
