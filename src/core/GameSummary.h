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

#include <cstdint>
#include <optional>
#include <string>
#include "core/Piece.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Outcome
    //-------------------------------------------------

    /**
     * @brief How a game ended for the user.
     */
    enum class Outcome : std::uint8_t
    {
        WIN, ///< The user won
        LOSS, ///< The user lost
        DRAW, ///< A draw
        UNKNOWN ///< Not finished, or the user did not play in the game
    };

    //-------------------------------------------------
    // GameSummary
    //-------------------------------------------------

    /**
     * @brief What the list of games shows about one game. The moves are in the PGN text and are read only when the
     *        user opens the game.
     */
    struct GameSummary
    {
        std::string source; ///< Where the game comes from, e.g. "chess.com"
        std::string id; ///< An id of the source that is the same for the same game
        std::string url; ///< The web page of the game
        std::string pgn; ///< The whole game as PGN text
        std::string white; ///< Name of the white player
        std::string black; ///< Name of the black player
        int whiteRating{ 0 }; ///< Rating of White, 0 if unknown
        int blackRating{ 0 }; ///< Rating of Black, 0 if unknown
        std::optional<Color> userColor; ///< The color of the user, if the user played in the game
        std::string result; ///< "1-0", "0-1", "1/2-1/2" or "*"
        std::string endReason; ///< Why it ended, in the words of the source, e.g. "resigned"
        Outcome outcome{ Outcome::UNKNOWN }; ///< The result from the point of view of the user
        std::string timeControl; ///< As in the PGN tag, e.g. "900+10", or "1/86400" for daily games
        std::string timeClass; ///< "bullet", "blitz", "rapid" or "daily"
        std::int64_t endTime{ 0 }; ///< When the game ended, in seconds since 1970 (UTC)
        std::optional<double> whiteAccuracy; ///< Accuracy of White in percent, if the source has it
        std::optional<double> blackAccuracy; ///< Accuracy of Black in percent, if the source has it
        std::string eco; ///< The ECO code, e.g. "B90", or empty
        std::string opening; ///< The name of the opening as the source calls it, or empty
        int moves{ 0 }; ///< The number of moves (a move is a move of White and one of Black)
        bool rated{ false }; ///< Whether it was a rated game
    };

    //-------------------------------------------------
    // Results
    //-------------------------------------------------

    /**
     * @brief Tells what a result code of chess.com means for the player who has it.
     *
     * "win" is a win. "checkmated", "timeout", "resigned", "lose" and "abandoned" are losses. "agreed", "repetition",
     * "stalemate", "insufficient", "50move" and "timevsinsufficient" are draws. Everything else is unknown.
     *
     * @param t_code The code, e.g. "resigned".
     *
     * @return WIN, LOSS, DRAW or UNKNOWN.
     */
    [[nodiscard]] Outcome classify_result(const std::string& t_code);

    /**
     * @brief Makes the PGN result of a game from the codes of the two players.
     *
     * @param t_whiteCode The result code of White.
     * @param t_blackCode The result code of Black.
     *
     * @return "1-0", "0-1", "1/2-1/2" or "*".
     */
    [[nodiscard]] std::string pgn_result(const std::string& t_whiteCode, const std::string& t_blackCode);

    /**
     * @brief Tells how a game ended for a player.
     *
     * @param t_result The PGN result: "1-0", "0-1", "1/2-1/2" or anything else.
     * @param t_color The color of the player.
     *
     * @return The outcome from the point of view of that player.
     */
    [[nodiscard]] Outcome outcome_for(const std::string& t_result, Color t_color);

    /**
     * @brief Describes why a game ended, in the language of the user.
     *
     * @param t_code The code of the loser or of a draw, e.g. "checkmated".
     *
     * @return A short text such as "Checkmate", or the code itself if it is not known.
     */
    [[nodiscard]] std::string describe_end_reason(const std::string& t_code);

    //-------------------------------------------------
    // Texts
    //-------------------------------------------------

    /**
     * @brief Writes the time control short, as it is on the list: "15+10", "3+2", "30 s", "1 d".
     *
     * @param t_timeControl The PGN value of the time control, e.g. "900+10" or "1/86400".
     *
     * @return The short text, or the value as it is if it cannot be read.
     */
    [[nodiscard]] std::string short_time_control(const std::string& t_timeControl);

    /**
     * @brief Writes a time of the day count as a date: year, month and day in UTC, e.g. "2026-11-25".
     *
     * @param t_seconds Seconds since 1970.
     *
     * @return The date, or an empty text if t_seconds is not positive.
     */
    [[nodiscard]] std::string format_game_date(std::int64_t t_seconds);

    /**
     * @brief Makes a readable name from the last part of an opening address of chess.com.
     *
     * "https://www.chess.com/openings/Sicilian-Defense-Najdorf-Variation" gives "Sicilian Defense Najdorf Variation".
     *
     * @param t_url The address.
     *
     * @return The name, or an empty text.
     */
    [[nodiscard]] std::string opening_from_url(const std::string& t_url);

    /**
     * @brief Counts the moves of a game in PGN text: the highest move number that is outside of comments.
     *
     * @param t_pgn The PGN text with the tags.
     *
     * @return The number of moves, 0 if there is none.
     */
    [[nodiscard]] int count_moves(const std::string& t_pgn);
} // namespace sg::chess::core
