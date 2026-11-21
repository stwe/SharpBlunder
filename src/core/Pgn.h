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

#include <string>
#include <string_view>
#include <vector>
#include "core/GameState.h"
#include "core/RulesProvider.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // PgnTag / PgnHeaders
    //-------------------------------------------------

    /**
     * @brief One tag of a PGN game, e.g. [White "Carlsen, Magnus"].
     */
    struct PgnTag
    {
        std::string name;   ///< The name, e.g. "White"
        std::string value;  ///< The value, e.g. "Carlsen, Magnus"
    };

    /**
     * @brief The tags of a PGN game, in the order of the file.
     *
     * The tags SetUp and FEN are not kept here: they follow from the start position of the game.
     */
    struct PgnHeaders
    {
        std::vector<PgnTag> tags;  ///< All tags except SetUp and FEN

        /**
         * @brief Creates the seven tags that every PGN game has, with values for a game that was analyzed here.
         *
         * @param t_date The date as "YYYY.MM.DD", or "????.??.??" if it is not known.
         *
         * @return The headers.
         */
        [[nodiscard]] static PgnHeaders MakeDefault(std::string_view t_date);

        /**
         * @brief Returns the value of a tag.
         *
         * @param t_name The name of the tag.
         *
         * @return The value, or an empty text if there is no such tag.
         */
        [[nodiscard]] std::string Get(std::string_view t_name) const;

        /**
         * @brief Sets the value of a tag. The tag is added at the end if it does not exist.
         *
         * @param t_name The name of the tag.
         * @param t_value The new value.
         */
        void Set(std::string_view t_name, std::string_view t_value);
    };

    //-------------------------------------------------
    // PgnGame
    //-------------------------------------------------

    /**
     * @brief A game read from PGN: its tags and its moves with variations and comments.
     */
    struct PgnGame
    {
        PgnHeaders headers;  ///< The tags
        GameState game;      ///< The moves. The current position is the start position.
    };

    //-------------------------------------------------
    // Writing and reading
    //-------------------------------------------------

    /**
     * @brief Writes a game as PGN text.
     *
     * The text has the seven tag roster, the other tags, the setup of the start position if it is not the standard
     * one, and the moves with their variations in parentheses, comments in braces and glyphs as $n. A comment on a
     * variation is written right after its opening parenthesis. A comment on the game is written in front of the
     * first move. The lines are at most 80 characters long, except for comments, which are never split.
     *
     * A closing brace in a comment cannot be written in PGN. It is replaced by a closing parenthesis.
     *
     * @param t_game The game.
     * @param t_headers The tags. Missing tags of the seven tag roster get the value "?" ("*" for the result).
     *
     * @return The PGN text, ending with a line break.
     */
    [[nodiscard]] std::string pgn_write(const GameState& t_game, const PgnHeaders& t_headers);

    /**
     * @brief Reads the first game of a PGN text.
     *
     * Understood are: tags, the setup with a FEN tag, moves in SAN (also with a move number glued to the move),
     * variations in parentheses, comments in braces and after a semicolon, numeric glyphs ($n) and the glyphs ! and ?
     * after a move. A comment right after an opening parenthesis, before the first move of the variation, becomes
     * the comment on the whole variation. Commands inside a comment, such as [%clk 0:03:00] of lichess, are kept apart
     * from the text. Further games in the text are ignored.
     *
     * The moves are played with the given rules, so they are checked: an illegal move is an error.
     *
     * @param t_text The PGN text.
     * @param t_rules The source of the rules. It must outlive the returned game.
     *
     * @return The game, at its start position.
     *
     * @throws std::runtime_error If the text is not valid PGN, with a message that tells what is wrong and where.
     */
    [[nodiscard]] PgnGame pgn_read(std::string_view t_text, RulesProvider& t_rules);
} // namespace sg::chess::core
