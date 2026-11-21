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

#include <array>
#include <optional>
#include "core/GameState.h"
#include "core/Move.h"
#include "core/Piece.h"
#include "core/Position.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // Material
    //-------------------------------------------------

    /**
     * @brief The pieces one side has captured.
     */
    struct CapturedPieces
    {
        std::array<int, 6> count{};  ///< How many pieces of each kind, indexed by PieceType

        /**
         * @brief Adds up the value of the captured pieces: pawn 1, knight and bishop 3, rook 5, queen 9.
         *
         * @return The points.
         */
        [[nodiscard]] int Points() const noexcept;
    };

    /**
     * @brief What each side has captured so far.
     */
    struct Material
    {
        CapturedPieces byWhite;  ///< The Black pieces that White has captured
        CapturedPieces byBlack;  ///< The White pieces that Black has captured

        /**
         * @brief Tells who is ahead in material.
         *
         * @return The points that White has captured minus the points that Black has captured. Positive means
         *         that White is ahead.
         */
        [[nodiscard]] int Balance() const noexcept { return byWhite.Points() - byBlack.Points(); }
    };

    /**
     * @brief Returns the value of a piece in points: pawn 1, knight and bishop 3, rook 5, queen 9, king 0.
     *
     * @param t_type The kind of piece.
     *
     * @return The points.
     */
    [[nodiscard]] int piece_points(PieceType t_type) noexcept;

    /**
     * @brief Finds the piece that a move captures.
     *
     * An en passant capture is recognized: a pawn that moves to the side onto an empty square.
     *
     * @param t_before The position before the move.
     * @param t_move The move. It must be legal in the position.
     *
     * @return The captured piece, or std::nullopt if the move does not capture.
     */
    [[nodiscard]] std::optional<Piece> captured_piece(const Position& t_before, const Move& t_move);

    /**
     * @brief Counts the captured pieces along the moves that led to the current position of a game.
     *
     * Only the moves of this line count, not the variations beside it. A pawn that was promoted and is captured
     * later counts as the piece it had become. A game that starts from a FEN counts from there.
     *
     * @param t_game The game.
     *
     * @return The captured pieces of both sides.
     */
    [[nodiscard]] Material captured_material(const GameState& t_game);
} // namespace sg::chess::core
