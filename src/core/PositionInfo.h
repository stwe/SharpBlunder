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
