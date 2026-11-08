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
