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
#include "core/Position.h"

namespace sg::chess::core
{
    /**
     * @brief Checks whether a position can exist in a game of chess, as far as the engine needs it.
     *
     * A FEN can be written correctly and still describe a position that cannot be played: without a king,
     * with a pawn on the last rank, or with the side that is not to move in check. Engines may behave strangely or
     * crash with such a position, so it is refused before it reaches one.
     *
     * The checks: exactly one king for each side, at most 8 pawns and 16 pieces for each side, no pawns on the first
     * or the last rank, and the king of the side that is not to move must not be attacked (that includes kings that
     * stand next to each other).
     *
     * @param t_position The position.
     *
     * @return std::nullopt if the position is fine, else a text that says what is wrong.
     */
    [[nodiscard]] std::optional<std::string> check_position(const Position& t_position);
} // namespace sg::chess::core
