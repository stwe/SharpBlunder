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
#include "core/Move.h"
#include "core/PositionInfo.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Standard Algebraic Notation
    //-------------------------------------------------

    /**
     * @brief Writes a move in Standard Algebraic Notation (SAN), e.g. "Nbd7", "exd5", "O-O" or "Qxf7#".
     *
     * @param t_before The position before the move, with its legal moves (needed to tell apart pieces
     *                 that can reach the same square).
     * @param t_move The move. It must be legal in t_before.
     * @param t_after The position after the move (needed for the check '+' and checkmate '#' suffix).
     *
     * @return The move in SAN.
     */
    [[nodiscard]] std::string to_san(const PositionInfo& t_before, const Move& t_move, const PositionInfo& t_after);
}
