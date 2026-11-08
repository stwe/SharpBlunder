#include <cstdlib>
#include "core/San.h"
#include "core/Square.h"

namespace
{
    // Do two moves of the same kind of piece end on the same square? Then the move needs a disambiguation.
    std::string disambiguation(const sg::chess::core::PositionInfo& t_before, const sg::chess::core::Move& t_move,
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
            san += disambiguation(t_before, t_move, piece->type);
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
