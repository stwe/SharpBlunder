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
