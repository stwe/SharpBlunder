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

    /**
     * @brief Finds the move that a text in Standard Algebraic Notation means.
     *
     * This is the reverse of to_san(). The text may have the suffixes of check and checkmate ("+", "#") and the glyphs
     * "!" and "?", which are ignored. Castling can be written with the letter O or with the digit 0. A promotion can
     * be written with or without "=" ("e8=Q" or "e8Q"). The text has to be exact: a missing letter of the piece or
     * a missing disambiguation that is needed makes the move ambiguous.
     *
     * @param t_position The position with its legal moves. The move must be one of them.
     * @param t_san The move in SAN.
     *
     * @return The move, or std::nullopt if the text is no legal move here or means more than one move.
     */
    [[nodiscard]] std::optional<Move> from_san(const PositionInfo& t_position, std::string_view t_san);
} // namespace sg::chess::core
