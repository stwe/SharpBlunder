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

namespace sg::chess::core
{
    //-------------------------------------------------
    // Score
    //-------------------------------------------------

    /**
     * @brief An evaluation of a position: either a value in centipawns or a forced mate.
     *
     * Stockfish reports the score from the point of view of the side to move. For the evaluation bar the score
     * is turned around so that positive always means "good for White", see FromSideToMove().
     */
    struct Score
    {
        /**
         * @brief The kind of the value.
         */
        enum class Kind
        {
            CENTIPAWNS, ///< value is a hundredth of a pawn
            MATE ///< value is the number of moves until mate; negative if the side in question is mated
        };

        Kind kind{ Kind::CENTIPAWNS }; ///< The kind of the value
        int value{ 0 }; ///< Centipawns or moves until mate

        /**
         * @brief Compares two scores.
         *
         * @return True if kind and value are equal.
         */
        [[nodiscard]] constexpr bool operator==(const Score&) const noexcept = default;

        /**
         * @brief Turns a score of the side to move into a score from the point of view of White.
         *
         * @param t_whiteToMove True if White is the side to move.
         *
         * @return The same score if White moves, the negated score if Black moves.
         */
        [[nodiscard]] Score FromSideToMove(bool t_whiteToMove) const noexcept;

        /**
         * @brief Tells how much of the evaluation bar belongs to White.
         *
         * Centipawns are mapped with a smooth curve: small advantages move the bar a lot,
         * big advantages only a little. A forced mate fills the whole bar. This score must be seen from White.
         *
         * @return A value from 0.0 (Black wins) to 1.0 (White wins). 0.5 is an equal position.
         */
        [[nodiscard]] double WhiteShare() const noexcept;

        /**
         * @brief Turns the score into centipawns, so two scores can be compared.
         *
         * A forced mate counts as a huge value. A shorter mate counts more than a longer one.
         * The sign stays: a mate for the other side is a huge negative value.
         *
         * @return The value in centipawns.
         */
        [[nodiscard]] int Centipawns() const noexcept;

        /**
         * @brief Describes the score in words for someone who does not read centipawns.
         *
         * This score must be seen from White. Examples: "Equal", "White is slightly better", "Black is winning",
         * "White mates in 3". The limits are listed in Score.cpp.
         *
         * @return The text.
         */
        [[nodiscard]] std::string Describe() const;

        /**
         * @brief Writes the score as short text for the bar and the list of lines.
         *
         * This score must be seen from White. A value of 35 centipawns gives "+0.35", a mate in three for Black "-M3".
         *
         * @return The text.
         */
        [[nodiscard]] std::string ToString() const;
    };
} // namespace sg::chess::core
