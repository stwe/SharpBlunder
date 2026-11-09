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
#include "core/Piece.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Move
    //-------------------------------------------------

    /**
     * @brief A move: from one square to another, with the piece type for a pawn promotion.
     *
     * A castling move is written as a king move of two squares (e1g1), exactly as in the UCI protocol.
     */
    struct Move
    {
        int from{ 0 };                       ///< Start square index
        int to{ 0 };                         ///< Target square index
        std::optional<PieceType> promotion;  ///< Set if a pawn is promoted

        [[nodiscard]] constexpr bool operator==(const Move&) const noexcept = default;

        /**
         * @brief Writes the move in UCI notation.
         *
         * @return The move, e.g. "e2e4" or "a7a8q".
         */
        [[nodiscard]] std::string ToUci() const;

        /**
         * @brief Parses a move in UCI notation.
         *
         * @param t_uci A move such as "e2e4" or "a7a8q".
         *
         * @return The move, or std::nullopt if the text is not a move.
         */
        [[nodiscard]] static std::optional<Move> FromUci(std::string_view t_uci);
    };
}
