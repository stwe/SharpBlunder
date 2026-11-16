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

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "core/Position.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Opening
    //-------------------------------------------------

    /**
     * @brief The name of an opening.
     */
    struct Opening
    {
        std::string eco;   ///< ECO code, e.g. "B90"
        std::string name;  ///< Name, e.g. "Sicilian Defense: Najdorf Variation"
    };

    //-------------------------------------------------
    // OpeningBook
    //-------------------------------------------------

    /**
     * @brief Knows the names of the openings and finds the one that belongs to a position.
     *
     * The names are looked up by position, not by the moves that led there. So an opening is also found
     * after a transposition (the same position reached by another order of moves).
     *
     * A position is compared by the placement of the pieces, the side to move and the castling rights. The en passant
     * square and the move counters are ignored: they do not change the name of an opening, and the engine and the
     * data may write the en passant square differently.
     */
    class OpeningBook
    {
    public:
        //-------------------------------------------------
        // Creation
        //-------------------------------------------------

        /**
         * @brief Reads the openings from tab separated text with the columns eco, name and epd.
         *
         * The first line is the header and is skipped. Empty lines are skipped as well. If a position occurs
         * more than once, the first entry counts.
         *
         * @param t_tsv The text, see assets/openings/openings.tsv.
         *
         * @return The opening book.
         *
         * @throws std::runtime_error If a line has fewer than three columns.
         */
        [[nodiscard]] static OpeningBook FromTsv(std::string_view t_tsv);

        /**
         * @brief Reads the openings from a file, see FromTsv().
         *
         * @param t_path The file.
         *
         * @return The opening book.
         *
         * @throws std::runtime_error If the file cannot be read or has a malformed line.
         */
        [[nodiscard]] static OpeningBook FromFile(const std::filesystem::path& t_path);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Finds the opening of a position.
         *
         * @param t_position The position.
         *
         * @return The opening, or std::nullopt if the position is not in the book.
         */
        [[nodiscard]] std::optional<Opening> Find(const Position& t_position) const;

        /**
         * @brief Returns the number of positions in the book.
         *
         * @return The number of entries.
         */
        [[nodiscard]] std::size_t Size() const noexcept { return m_openings.size(); }

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::unordered_map<std::string, Opening> m_openings;  ///< Key: pieces, side to move, castling rights
    };
}
