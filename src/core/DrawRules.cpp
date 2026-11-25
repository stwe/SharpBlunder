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

#include <string>
#include "core/DrawRules.h"
#include "core/Square.h"

namespace
{
    // The positions of 100 and 150 halfmoves without a capture or a pawn move.
    constexpr int FIFTY_MOVE_HALFMOVES{ 100 };
    constexpr int SEVENTY_FIVE_MOVE_HALFMOVES{ 150 };

    constexpr int THREEFOLD{ 3 };
    constexpr int FIVEFOLD{ 5 };

    // What decides whether two positions are the same: the pieces, the side to move and the castling rights. These
    // are the first three fields of the FEN.
    std::string RepetitionKey(const sg::chess::core::Position& t_position)
    {
        const std::string fen{ t_position.ToFen() };

        std::size_t end{ 0 };
        for (int field{ 0 }; field < 3 && end != std::string::npos; ++field)
        {
            end = fen.find(' ', end == 0 ? 0 : end + 1);
        }

        return fen.substr(0, end);
    }

    // The color of a square: 0 for the dark squares (a1 is dark), 1 for the light ones.
    int SquareColor(const int t_square)
    {
        return (sg::chess::core::file_of(t_square) + sg::chess::core::rank_of(t_square)) % 2;
    }
} // namespace

//-------------------------------------------------
// Rules
//-------------------------------------------------

bool sg::chess::core::is_insufficient_material(const Position& t_position)
{
    int minorPieces{ 0 };
    int bishops{ 0 };
    int bishopsOnLight{ 0 };

    for (int square{ 0 }; square < SQUARE_COUNT; ++square)
    {
        const auto piece{ t_position.At(square) };

        if (!piece)
        {
            continue;
        }

        switch (piece->type)
        {
        case PieceType::KING:
            break;
        case PieceType::KNIGHT:
            ++minorPieces;
            break;
        case PieceType::BISHOP:
            ++minorPieces;
            ++bishops;
            bishopsOnLight += SquareColor(square);
            break;
        default:
            // A pawn, a rook or a queen: checkmate is possible.
            return false;
        }
    }

    // The kings alone, or one minor piece.
    if (minorPieces <= 1)
    {
        return true;
    }

    // Only bishops, all of them on the same color.
    return bishops == minorPieces && (bishopsOnLight == 0 || bishopsOnLight == bishops);
}

int sg::chess::core::repetition_count(const GameState& t_game, const GameState::NodeId t_id)
{
    const GameState::Node& node{ t_game.At(t_id) };
    const std::string key{ RepetitionKey(node.info.position) };

    // Positions before the last capture or pawn move cannot come back. Every halfmove is one step up the line.
    const int steps{ node.info.position.HalfmoveClock() };

    int count{ 1 };
    std::optional<GameState::NodeId> parent{ node.parent };

    for (int step{ 1 }; step <= steps && parent; ++step)
    {
        const GameState::Node& earlier{ t_game.At(*parent) };

        // The same side must be to move, so only every second position can match.
        if (step % 2 == 0 && RepetitionKey(earlier.info.position) == key)
        {
            ++count;
        }

        parent = earlier.parent;
    }

    return count;
}

std::optional<sg::chess::core::DrawReason> sg::chess::core::find_draw(const PositionInfo& t_info,
                                                                       const int t_repetitions)
{
    if (t_info.IsCheckmate() || t_info.IsStalemate())
    {
        return std::nullopt;
    }

    const int halfmoves{ t_info.position.HalfmoveClock() };

    if (is_insufficient_material(t_info.position))
    {
        return DrawReason::INSUFFICIENT_MATERIAL;
    }

    if (t_repetitions >= FIVEFOLD)
    {
        return DrawReason::FIVEFOLD_REPETITION;
    }

    if (halfmoves >= SEVENTY_FIVE_MOVE_HALFMOVES)
    {
        return DrawReason::SEVENTY_FIVE_MOVES;
    }

    if (t_repetitions >= THREEFOLD)
    {
        return DrawReason::THREEFOLD_REPETITION;
    }

    if (halfmoves >= FIFTY_MOVE_HALFMOVES)
    {
        return DrawReason::FIFTY_MOVES;
    }

    return std::nullopt;
}

//-------------------------------------------------
// Texts
//-------------------------------------------------

bool sg::chess::core::is_automatic_draw(const DrawReason t_reason) noexcept
{
    return t_reason == DrawReason::INSUFFICIENT_MATERIAL || t_reason == DrawReason::FIVEFOLD_REPETITION
           || t_reason == DrawReason::SEVENTY_FIVE_MOVES;
}

std::string_view sg::chess::core::describe_draw(const DrawReason t_reason) noexcept
{
    switch (t_reason)
    {
    case DrawReason::INSUFFICIENT_MATERIAL:
        return "insufficient material";
    case DrawReason::FIVEFOLD_REPETITION:
        return "fivefold repetition";
    case DrawReason::SEVENTY_FIVE_MOVES:
        return "75-move rule";
    case DrawReason::THREEFOLD_REPETITION:
        return "threefold repetition";
    case DrawReason::FIFTY_MOVES:
        return "50-move rule";
    }

    return "";
}
