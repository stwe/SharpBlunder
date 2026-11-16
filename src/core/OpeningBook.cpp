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

#include <fstream>
#include <sstream>
#include <stdexcept>
#include "core/OpeningBook.h"
#include "core/Util.h"

namespace
{
    constexpr char COLUMN_SEPARATOR{ '\t' };
    constexpr char FIELD_SEPARATOR{ ' ' };
    constexpr std::size_t KEY_FIELDS{ 3 };  // pieces, side to move, castling rights

    // Keeps the first fields of a FEN or EPD: that is what identifies a position here.
    std::string MakeKey(const std::string_view t_fen)
    {
        std::size_t end{ 0 };

        for (std::size_t field{ 0 }; field < KEY_FIELDS; ++field)
        {
            end = t_fen.find(FIELD_SEPARATOR, end);
            if (end == std::string_view::npos)
            {
                return std::string{ t_fen };
            }

            if (field + 1 < KEY_FIELDS)
            {
                ++end;
            }
        }

        return std::string{ t_fen.substr(0, end) };
    }
}

//-------------------------------------------------
// Creation
//-------------------------------------------------

sg::chess::core::OpeningBook sg::chess::core::OpeningBook::FromTsv(const std::string_view t_tsv)
{
    OpeningBook book;

    std::size_t lineNumber{ 0 };
    for (const std::string& rawLine : split_str(t_tsv, '\n'))
    {
        ++lineNumber;

        // The header and empty lines carry no opening. A file with Windows line endings is read as well.
        std::string_view line{ rawLine };
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }

        if (lineNumber == 1 || line.empty())
        {
            continue;
        }

        const std::vector<std::string> columns{ split_str(line, COLUMN_SEPARATOR) };
        if (columns.size() < 3)
        {
            throw std::runtime_error("Opening data: line " + std::to_string(lineNumber) +
                                     " has fewer than three columns");
        }

        book.m_openings.emplace(MakeKey(columns[2]), Opening{ columns[0], columns[1] });
    }

    return book;
}

sg::chess::core::OpeningBook sg::chess::core::OpeningBook::FromFile(const std::filesystem::path& t_path)
{
    std::ifstream file{ t_path };
    if (!file)
    {
        throw std::runtime_error("Cannot read the opening names from " + t_path.string());
    }

    std::ostringstream text;
    text << file.rdbuf();

    return FromTsv(text.str());
}

//-------------------------------------------------
// Query
//-------------------------------------------------

std::optional<sg::chess::core::Opening> sg::chess::core::OpeningBook::Find(const Position& t_position) const
{
    const auto it{ m_openings.find(MakeKey(t_position.ToFen())) };

    return it != m_openings.end() ? std::optional<Opening>{ it->second } : std::nullopt;
}
