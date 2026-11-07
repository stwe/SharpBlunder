#include <stdexcept>
#include <string>
#include "core/Position.h"

//-------------------------------------------------
// Creation
//-------------------------------------------------

sg::chess::core::Position sg::chess::core::Position::FromFen(const std::string_view t_fen)
{
    const auto placement{ t_fen.substr(0, t_fen.find(' ')) };

    Position position;
    int rank{ BOARD_SIZE - 1 };
    int file{ 0 };

    for (const char c : placement)
    {
        if (c == '/')
        {
            if (file != BOARD_SIZE || rank == 0)
            {
                throw std::invalid_argument("Invalid FEN (rank is incomplete or there are too many ranks): " +
                                            std::string(t_fen));
            }

            --rank;
            file = 0;
        }
        else if (c >= '1' && c <= '8')
        {
            file += c - '0';
            if (file > BOARD_SIZE)
            {
                throw std::invalid_argument("Invalid FEN (rank has more than 8 squares): " + std::string(t_fen));
            }
        }
        else
        {
            const auto piece{ from_fen_char(c) };
            if (!piece || file >= BOARD_SIZE)
            {
                throw std::invalid_argument("Invalid FEN (unexpected character): " + std::string(t_fen));
            }

            position.m_squares[static_cast<std::size_t>(Index(file, rank))] = piece;
            ++file;
        }
    }

    if (rank != 0 || file != BOARD_SIZE)
    {
        throw std::invalid_argument("Invalid FEN (board is incomplete): " + std::string(t_fen));
    }

    return position;
}

//-------------------------------------------------
// Access
//-------------------------------------------------

std::optional<sg::chess::core::Piece> sg::chess::core::Position::At(const int t_file, const int t_rank) const
{
    if (t_file < 0 || t_file >= BOARD_SIZE || t_rank < 0 || t_rank >= BOARD_SIZE)
    {
        throw std::out_of_range("Square is outside the board");
    }

    return m_squares[static_cast<std::size_t>(Index(t_file, t_rank))];
}
