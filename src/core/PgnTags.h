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

#include <optional>
#include <string>
#include <string_view>
#include "core/OpeningBook.h"
#include "core/Pgn.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Date
    //-------------------------------------------------

    /**
     * @brief Brings a date into the form that PGN wants: "YYYY.MM.DD", with "??" for a part that is not known.
     *
     * Understood are the year first ("2026.11.24", "2026-11-24", "2026/11/24") and the day first ("24.11.2026"). The
     * year may be "????", the month and the day may be "??" (also written with a single "?"). The month must be 1 to
     * 12 and the day must exist in that month. A day without a month is refused.
     *
     * @param t_text The text.
     *
     * @return The date in PGN form, or std::nullopt if the text is not a date.
     */
    [[nodiscard]] std::optional<std::string> normalize_date(std::string_view t_text);

    //-------------------------------------------------
    // Opening
    //-------------------------------------------------

    /**
     * @brief Checks whether a text is an ECO code: a letter from A to E and two digits, e.g. "B90".
     *
     * @param t_text The text.
     *
     * @return true if it is one.
     */
    [[nodiscard]] bool is_eco_code(std::string_view t_text);

    /**
     * @brief Writes the opening of a game into the tags ECO, Opening, Variation and SubVariation.
     *
     * The name is split the way the lichess names are built: "Sicilian Defense: Najdorf Variation, English Attack"
     * gives the opening "Sicilian Defense", the variation "Najdorf Variation" and the sub-variation "English Attack".
     * Tags that the name has no part for are removed, so that no old value stays.
     *
     * @param t_headers The tags of the game.
     * @param t_opening The opening.
     */
    void set_opening_tags(PgnHeaders& t_headers, const Opening& t_opening);

    /**
     * @brief Removes the tags ECO, Opening, Variation and SubVariation.
     *
     * @param t_headers The tags of the game.
     */
    void clear_opening_tags(PgnHeaders& t_headers);
} // namespace sg::chess::core
