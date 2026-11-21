// This file is part of the SharpBlunder project.
//
// Copyright (c) 2026 stwe <https://github.com/stwe/SharpBlunder>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <array>
#include <cstdlib>
#include "core/PositionCheck.h"

namespace
{
    using sg::chess::core::Color;
    using sg::chess::core::PieceType;
    using sg::chess::core::Position;

    constexpr int MAX_PAWNS{ 8 };
    constexpr int MAX_PIECES{ 16 };

    constexpr std::array<std::array<int, 2>, 8> KNIGHT_STEPS{ { { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 },
                                                              { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } } };
    constexpr std::array<std::array<int, 2>, 8> KING_STEPS{ { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
                                                            { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } } };

    bool OnBoard(const int t_file, const int t_rank)
    {
        return t_file >= 0 && t_file < 8 && t_rank >= 0 && t_rank < 8;
    }

    // Is there a piece of the given kind and color at this place?
    bool Has(const Position& t_position, const int t_file, const int t_rank, const PieceType t_type,
             const Color t_color)
    {
        if (!OnBoard(t_file, t_rank))
        {
            return false;
        }

        const auto piece{ t_position.At(t_file, t_rank) };

        return piece && piece->type == t_type && piece->color == t_color;
    }

    // Does a piece of the given color attack a square? Sliders are followed until they meet something.
    bool IsAttacked(const Position& t_position, const int t_file, const int t_rank, const Color t_by)
    {
        // A pawn attacks the squares in front of it, diagonally. White pawns move up, Black pawns down.
        const int pawnRank{ t_by == Color::WHITE ? t_rank - 1 : t_rank + 1 };
        if (Has(t_position, t_file - 1, pawnRank, PieceType::PAWN, t_by) ||
            Has(t_position, t_file + 1, pawnRank, PieceType::PAWN, t_by))
        {
            return true;
        }

        for (const auto& step : KNIGHT_STEPS)
        {
            if (Has(t_position, t_file + step[0], t_rank + step[1], PieceType::KNIGHT, t_by))
            {
                return true;
            }
        }

        for (const auto& step : KING_STEPS)
        {
            if (Has(t_position, t_file + step[0], t_rank + step[1], PieceType::KING, t_by))
            {
                return true;
            }

            // Along this direction: a rook or a queen on a line, a bishop or a queen on a diagonal.
            const bool diagonal{ step[0] != 0 && step[1] != 0 };

            for (int f{ t_file + step[0] }, r{ t_rank + step[1] }; OnBoard(f, r); f += step[0], r += step[1])
            {
                const auto piece{ t_position.At(f, r) };
                if (!piece)
                {
                    continue;
                }

                const PieceType slider{ diagonal ? PieceType::BISHOP : PieceType::ROOK };
                if (piece->color == t_by && (piece->type == slider || piece->type == PieceType::QUEEN))
                {
                    return true;
                }

                break;
            }
        }

        return false;
    }
} // namespace

std::optional<std::string> sg::chess::core::check_position(const Position& t_position)
{
    std::array<int, 2> kings{};
    std::array<int, 2> pawns{};
    std::array<int, 2> pieces{};

    for (int square{ 0 }; square < SQUARE_COUNT; ++square)
    {
        const auto piece{ t_position.At(square) };
        if (!piece)
        {
            continue;
        }

        const auto side{ static_cast<std::size_t>(piece->color) };
        ++pieces[side];

        if (piece->type == PieceType::KING)
        {
            ++kings[side];
        }
        else if (piece->type == PieceType::PAWN)
        {
            ++pawns[side];

            if (rank_of(square) == 0 || rank_of(square) == BOARD_SIZE - 1)
            {
                return "A pawn cannot be on the first or the last rank.";
            }
        }
    }

    for (const Color color : { Color::WHITE, Color::BLACK })
    {
        const auto side{ static_cast<std::size_t>(color) };
        const std::string name{ color == Color::WHITE ? "White" : "Black" };

        if (kings[side] != 1)
        {
            return name + " needs exactly one king.";
        }

        if (pawns[side] > MAX_PAWNS || pieces[side] > MAX_PIECES)
        {
            return name + " has too many pieces.";
        }
    }

    // The side that has just moved cannot have left its king in check.
    const Color mover{ t_position.SideToMove() == Color::WHITE ? Color::BLACK : Color::WHITE };
    const int kingSquare{ *t_position.KingSquare(mover) };

    if (IsAttacked(t_position, file_of(kingSquare), rank_of(kingSquare), t_position.SideToMove()))
    {
        return std::string{ mover == Color::WHITE ? "White" : "Black" } +
               " is in check, but it is not their turn. Is the side to move right?";
    }

    return std::nullopt;
}
