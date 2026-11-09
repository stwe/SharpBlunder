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

#include <algorithm>
#include <vector>
#include "core/Move.h"
#include "core/Position.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // PositionInfo
    //-------------------------------------------------

    /**
     * @brief A position together with what the rules say about it: the legal moves and whether the king is in check.
     */
    struct PositionInfo
    {
        Position position;             ///< The position
        std::vector<Move> legalMoves;  ///< All legal moves of the side to move
        bool inCheck{ false };         ///< True if the side to move is in check

        /**
         * @brief Tells whether the side to move is checkmated.
         *
         * @return True if in check and there is no legal move.
         */
        [[nodiscard]] bool IsCheckmate() const noexcept { return inCheck && legalMoves.empty(); }

        /**
         * @brief Tells whether the side to move is stalemated.
         *
         * @return True if not in check and there is no legal move.
         */
        [[nodiscard]] bool IsStalemate() const noexcept { return !inCheck && legalMoves.empty(); }

        /**
         * @brief Tells whether a move is legal in this position.
         *
         * @param t_move The move.
         *
         * @return True if the move is one of the legal moves.
         */
        [[nodiscard]] bool IsLegal(const Move& t_move) const
        {
            return std::ranges::find(legalMoves, t_move) != legalMoves.end();
        }
    };
}
