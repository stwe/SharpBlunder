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

#include <charconv>
#include <stdexcept>
#include <vector>
#include "core/Position.h"

namespace
{
    [[noreturn]] void Fail(const std::string_view t_reason, const std::string_view t_fen)
    {
        throw std::invalid_argument("Invalid FEN (" + std::string(t_reason) + "): " + std::string(t_fen));
    }

    std::vector<std::string_view> SplitFields(const std::string_view t_fen)
    {
        std::vector<std::string_view> fields;

        std::size_t start{ 0 };
        while (start < t_fen.size())
        {
            const auto space{ t_fen.find(' ', start) };
            const auto end{ space == std::string_view::npos ? t_fen.size() : space };

            if (end > start)
            {
                fields.push_back(t_fen.substr(start, end - start));
            }

            start = end + 1;
        }

        return fields;
    }

    int ParseNumber(const std::string_view t_text, const std::string_view t_fen)
    {
        int value{ 0 };
        const auto [end, error]{ std::from_chars(t_text.data(), t_text.data() + t_text.size(), value) };

        if (error != std::errc{} || end != t_text.data() + t_text.size() || value < 0)
        {
            Fail("not a number: " + std::string(t_text), t_fen);
        }

        return value;
    }
}

//-------------------------------------------------
// Creation
//-------------------------------------------------

sg::chess::core::Position sg::chess::core::Position::FromFen(const std::string_view t_fen)
{
    const auto fields{ SplitFields(t_fen) };
    if (fields.empty() || fields.size() > 6)
    {
        Fail("expected 1 to 6 fields", t_fen);
    }

    Position position;

    // Field 1: piece placement
    int rank{ BOARD_SIZE - 1 };
    int file{ 0 };

    for (const char c : fields[0])
    {
        if (c == '/')
        {
            if (file != BOARD_SIZE || rank == 0)
            {
                Fail("rank is incomplete or there are too many ranks", t_fen);
            }

            --rank;
            file = 0;
        }
        else if (c >= '1' && c <= '8')
        {
            file += c - '0';
            if (file > BOARD_SIZE)
            {
                Fail("rank has more than 8 squares", t_fen);
            }
        }
        else
        {
            const auto piece{ from_fen_char(c) };
            if (!piece || file >= BOARD_SIZE)
            {
                Fail("unexpected character in the piece placement", t_fen);
            }

            position.m_squares[static_cast<std::size_t>(square_index(file, rank))] = piece;
            ++file;
        }
    }

    if (rank != 0 || file != BOARD_SIZE)
    {
        Fail("board is incomplete", t_fen);
    }

    // Field 2: side to move
    if (fields.size() > 1)
    {
        if (fields[1] == "w")
        {
            position.m_sideToMove = Color::WHITE;
        }
        else if (fields[1] == "b")
        {
            position.m_sideToMove = Color::BLACK;
        }
        else
        {
            Fail("side to move must be 'w' or 'b'", t_fen);
        }
    }

    // Field 3: castling rights
    if (fields.size() > 2)
    {
        const bool valid{ fields[2] == "-" || fields[2].find_first_not_of("KQkq") == std::string_view::npos };
        if (!valid)
        {
            Fail("invalid castling rights", t_fen);
        }

        position.m_castling = std::string(fields[2]);
    }

    // Field 4: en passant square
    if (fields.size() > 3)
    {
        if (fields[3] != "-" && !square_from_name(fields[3]))
        {
            Fail("invalid en passant square", t_fen);
        }

        position.m_enPassant = std::string(fields[3]);
    }

    // Field 5 and 6: move counters
    if (fields.size() > 4)
    {
        position.m_halfmoveClock = ParseNumber(fields[4], t_fen);
    }

    if (fields.size() > 5)
    {
        position.m_fullmoveNumber = ParseNumber(fields[5], t_fen);
        if (position.m_fullmoveNumber < 1)
        {
            Fail("fullmove number must be at least 1", t_fen);
        }
    }

    return position;
}

//-------------------------------------------------
// Access
//-------------------------------------------------

std::string sg::chess::core::Position::ToFen() const
{
    std::string fen;

    for (int rank{ BOARD_SIZE - 1 }; rank >= 0; --rank)
    {
        int emptySquares{ 0 };

        for (int file{ 0 }; file < BOARD_SIZE; ++file)
        {
            const auto& piece{ m_squares[static_cast<std::size_t>(square_index(file, rank))] };
            if (!piece)
            {
                ++emptySquares;
                continue;
            }

            if (emptySquares > 0)
            {
                fen += static_cast<char>('0' + emptySquares);
                emptySquares = 0;
            }

            fen += to_fen_char(*piece);
        }

        if (emptySquares > 0)
        {
            fen += static_cast<char>('0' + emptySquares);
        }

        if (rank > 0)
        {
            fen += '/';
        }
    }

    fen += m_sideToMove == Color::WHITE ? " w " : " b ";
    fen += m_castling + ' ' + m_enPassant + ' ' + std::to_string(m_halfmoveClock) + ' ' + std::to_string(m_fullmoveNumber);

    return fen;
}

std::optional<sg::chess::core::Piece> sg::chess::core::Position::At(const int t_file, const int t_rank) const
{
    if (t_file < 0 || t_file >= BOARD_SIZE || t_rank < 0 || t_rank >= BOARD_SIZE)
    {
        throw std::out_of_range("Square is outside the board");
    }

    return m_squares[static_cast<std::size_t>(square_index(t_file, t_rank))];
}

std::optional<sg::chess::core::Piece> sg::chess::core::Position::At(const int t_square) const
{
    if (t_square < 0 || t_square >= SQUARE_COUNT)
    {
        throw std::out_of_range("Square is outside the board");
    }

    return m_squares[static_cast<std::size_t>(t_square)];
}

std::optional<int> sg::chess::core::Position::KingSquare(const Color t_color) const noexcept
{
    for (int square{ 0 }; square < SQUARE_COUNT; ++square)
    {
        if (m_squares[static_cast<std::size_t>(square)] == Piece{ PieceType::KING, t_color })
        {
            return square;
        }
    }

    return std::nullopt;
}
