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

#include <cstdlib>
#include "core/San.h"
#include "core/Square.h"

namespace
{
    // Do two moves of the same kind of piece end on the same square? Then the move needs a disambiguation.
    std::string Disambiguation(const sg::chess::core::PositionInfo& t_before, const sg::chess::core::Move& t_move,
                               const sg::chess::core::PieceType t_type)
    {
        bool ambiguous{ false };
        bool sameFile{ false };
        bool sameRank{ false };

        for (const sg::chess::core::Move& other : t_before.legalMoves)
        {
            const auto piece{ t_before.position.At(other.from) };

            if (other.to != t_move.to || other.from == t_move.from || !piece || piece->type != t_type)
            {
                continue;
            }

            ambiguous = true;
            sameFile = sameFile || sg::chess::core::file_of(other.from) == sg::chess::core::file_of(t_move.from);
            sameRank = sameRank || sg::chess::core::rank_of(other.from) == sg::chess::core::rank_of(t_move.from);
        }

        if (!ambiguous)
        {
            return "";
        }

        const std::string name{ sg::chess::core::square_name(t_move.from) };

        if (!sameFile)
        {
            return name.substr(0, 1);   // the file is enough: Nbd7
        }

        if (!sameRank)
        {
            return name.substr(1, 1);   // the rank is enough: N1d2
        }

        return name;                    // both are needed: Qa1b2
    }
}

//-------------------------------------------------
// Standard Algebraic Notation
//-------------------------------------------------

std::string sg::chess::core::to_san(const PositionInfo& t_before, const Move& t_move, const PositionInfo& t_after)
{
    const auto piece{ t_before.position.At(t_move.from) };
    if (!piece)
    {
        return t_move.ToUci();  // cannot happen for a legal move
    }

    std::string san;

    const bool isCastling{ piece->type == PieceType::KING && std::abs(file_of(t_move.to) - file_of(t_move.from)) == 2 };

    if (isCastling)
    {
        san = file_of(t_move.to) > file_of(t_move.from) ? "O-O" : "O-O-O";
    }
    else
    {
        // A pawn that changes the file captures, also en passant (the target square is empty then).
        const bool isCapture{ t_before.position.At(t_move.to).has_value() ||
                              (piece->type == PieceType::PAWN && file_of(t_move.from) != file_of(t_move.to)) };

        if (piece->type == PieceType::PAWN)
        {
            if (isCapture)
            {
                san += square_name(t_move.from)[0];
            }
        }
        else
        {
            san += type_letter(piece->type);
            san += Disambiguation(t_before, t_move, piece->type);
        }

        if (isCapture)
        {
            san += 'x';
        }

        san += square_name(t_move.to);

        if (t_move.promotion)
        {
            san += '=';
            san += type_letter(*t_move.promotion);
        }
    }

    if (t_after.IsCheckmate())
    {
        san += '#';
    }
    else if (t_after.inCheck)
    {
        san += '+';
    }

    return san;
}

std::optional<sg::chess::core::Move> sg::chess::core::from_san(const PositionInfo& t_position, std::string_view t_san)
{
    std::string_view san{ t_san };

    // Check, checkmate and the glyphs for good and bad moves do not change the move.
    while (!san.empty() && (san.back() == '+' || san.back() == '#' || san.back() == '!' || san.back() == '?'))
    {
        san.remove_suffix(1);
    }

    if (san.empty())
    {
        return std::nullopt;
    }

    // Castling: the letter O, or the digit 0 that some programs write.
    if (san.front() == 'O' || san.front() == '0')
    {
        const bool shortCastling{ san == "O-O" || san == "0-0" };
        const bool longCastling{ san == "O-O-O" || san == "0-0-0" };
        if (!shortCastling && !longCastling)
        {
            return std::nullopt;
        }

        for (const Move& move : t_position.legalMoves)
        {
            const auto piece{ t_position.position.At(move.from) };
            const bool isKingMove{ piece && piece->type == PieceType::KING &&
                                   std::abs(file_of(move.to) - file_of(move.from)) == 2 };

            if (isKingMove && (file_of(move.to) > file_of(move.from)) == shortCastling)
            {
                return move;
            }
        }

        return std::nullopt;
    }

    // The piece: a capital letter. No letter means a pawn.
    PieceType type{ PieceType::PAWN };
    if (std::string_view{ "KQRBN" }.find(san.front()) != std::string_view::npos)
    {
        type = from_fen_char(san.front())->type;
        san.remove_prefix(1);
    }

    // A letter at the end is the piece of a promotion, with or without "=".
    std::optional<PieceType> promotion;
    if (san.size() >= 2 && std::string_view{ "QRBNqrbn" }.find(san.back()) != std::string_view::npos)
    {
        promotion = from_fen_char(san.back())->type;
        san.remove_suffix(1);

        if (!san.empty() && san.back() == '=')
        {
            san.remove_suffix(1);
        }
    }

    // The target square are the last two characters.
    if (san.size() < 2)
    {
        return std::nullopt;
    }

    const auto target{ square_from_name(san.substr(san.size() - 2)) };
    if (!target)
    {
        return std::nullopt;
    }

    san.remove_suffix(2);

    if (!san.empty() && san.back() == 'x')
    {
        san.remove_suffix(1);
    }

    // What is left says where the piece comes from: a file, a rank, or both. The pawn that captures is told by its file.
    std::optional<int> fromFile;
    std::optional<int> fromRank;

    for (const char c : san)
    {
        if (c >= 'a' && c <= 'h')
        {
            fromFile = c - 'a';
        }
        else if (c >= '1' && c <= '8')
        {
            fromRank = c - '1';
        }
        else
        {
            return std::nullopt;
        }
    }

    std::optional<Move> found;

    for (const Move& move : t_position.legalMoves)
    {
        const auto piece{ t_position.position.At(move.from) };

        const bool matches{ piece && piece->type == type && move.to == *target && move.promotion == promotion &&
                            (!fromFile || file_of(move.from) == *fromFile) &&
                            (!fromRank || rank_of(move.from) == *fromRank) };
        if (!matches)
        {
            continue;
        }

        // A second move that fits: the text does not tell which piece is meant.
        if (found)
        {
            return std::nullopt;
        }

        found = move;
    }

    return found;
}
