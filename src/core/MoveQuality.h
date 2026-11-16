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

namespace sg::chess::core
{
    //-------------------------------------------------
    // MoveQuality
    //-------------------------------------------------

    /**
     * @brief How good a move is compared to the best move the engine found.
     */
    enum class MoveQuality
    {
        GOOD, ///< The best move, or almost as good
        OKAY, ///< Playable, but clearly weaker than the best move
        WEAK ///< Much weaker than the best move
    };

    /**
     * @brief Rates a move by what it loses against the best move.
     *
     * Up to half a pawn is good, up to one and a half pawns is okay, more is weak.
     *
     * @param t_lossCentipawns How much worse the move is than the best one, in centipawns. Zero or less
     *                         (the engine sometimes rates a second line a little higher) counts as good.
     *
     * @return The quality.
     */
    [[nodiscard]] MoveQuality classify_loss(int t_lossCentipawns) noexcept;
} // namespace sg::chess::core
