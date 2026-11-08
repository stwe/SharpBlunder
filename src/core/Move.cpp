#include "core/Move.h"
#include "core/Square.h"

//-------------------------------------------------
// Move
//-------------------------------------------------

std::string sg::chess::core::Move::ToUci() const
{
    std::string uci{ square_name(from) + square_name(to) };

    if (promotion)
    {
        uci += static_cast<char>(type_letter(*promotion) - 'A' + 'a');
    }

    return uci;
}

std::optional<sg::chess::core::Move> sg::chess::core::Move::FromUci(const std::string_view t_uci)
{
    if (t_uci.size() != 4 && t_uci.size() != 5)
    {
        return std::nullopt;
    }

    const auto from{ square_from_name(t_uci.substr(0, 2)) };
    const auto to{ square_from_name(t_uci.substr(2, 2)) };
    if (!from || !to)
    {
        return std::nullopt;
    }

    Move move{ *from, *to, std::nullopt };

    if (t_uci.size() == 5)
    {
        const char letter{ t_uci[4] };
        const auto piece{ from_fen_char(letter) };

        // Only lower case q, r, b and n are valid promotion letters.
        const bool valid{ letter >= 'a' && letter <= 'z' && piece &&
                          piece->type != PieceType::KING && piece->type != PieceType::PAWN };
        if (!valid)
        {
            return std::nullopt;
        }

        move.promotion = piece->type;
    }

    return move;
}
