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
#include <vector>
#include "core/GameSummary.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // ParsedMonth
    //-------------------------------------------------

    /**
     * @brief The games of one month, as the published-data API of chess.com gives them.
     */
    struct ParsedMonth
    {
        std::vector<GameSummary> games; ///< The games that the program can show
        int skippedVariants{ 0 }; ///< Games of a variant (Chess960, ...) that the program cannot show
        int skippedBroken{ 0 }; ///< Games with missing or unusable data
    };

    //-------------------------------------------------
    // Names
    //-------------------------------------------------

    /**
     * @brief Tells whether a text can be a user name of chess.com.
     *
     * The name is put into a web address, so only letters, digits, "_" and "-" are allowed, and not more than 50.
     *
     * @param t_name The name.
     *
     * @return True if the name is allowed.
     */
    [[nodiscard]] bool is_valid_chesscom_username(const std::string& t_name);

    //-------------------------------------------------
    // Parsing
    //-------------------------------------------------

    /**
     * @brief Reads the list of archives: {"archives": [<one address for each month>, ...]}.
     *
     * @param t_json The JSON text.
     *
     * @return The addresses in the order of the answer (oldest first), or std::nullopt if it is not such a list.
     */
    [[nodiscard]] std::optional<std::vector<std::string>> parse_chesscom_archives(const std::string& t_json);

    /**
     * @brief Reads the games of one month: {"games": [...]}.
     *
     * Games of a variant (the "rules" are not "chess") and games without a PGN text are skipped and counted. The
     * games come in the order of the answer.
     *
     * @param t_json The JSON text.
     * @param t_user The user name, to find out which side the user played. Upper and lower case do not matter.
     *
     * @return The games, or std::nullopt if the text is not such an answer.
     */
    [[nodiscard]] std::optional<ParsedMonth> parse_chesscom_month(const std::string& t_json, const std::string& t_user);
} // namespace sg::chess::core
