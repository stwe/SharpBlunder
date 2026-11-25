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

#include <cstdint>
#include <optional>
#include <string_view>
#include "core/GameState.h"
#include "core/Position.h"
#include "core/PositionInfo.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // DrawReason
    //-------------------------------------------------

    /**
     * @brief The reasons why a game can be a draw, besides stalemate.
     *
     * Following the FIDE laws: with insufficient material, fivefold repetition and 75 moves without a capture or a
     * pawn move the game is drawn at once. With threefold repetition and 50 moves a player may claim the draw.
     */
    enum class DrawReason : std::uint8_t
    {
        INSUFFICIENT_MATERIAL,  ///< No series of legal moves can lead to checkmate (dead position)
        FIVEFOLD_REPETITION,    ///< The same position has occurred five times
        SEVENTY_FIVE_MOVES,     ///< 75 moves (150 halfmoves) without a capture or a pawn move
        THREEFOLD_REPETITION,   ///< The same position has occurred three times: a player may claim a draw
        FIFTY_MOVES             ///< 50 moves (100 halfmoves) without a capture or a pawn move: a draw can be claimed
    };

    //-------------------------------------------------
    // Rules
    //-------------------------------------------------

    /**
     * @brief Tells whether neither side can give checkmate by any series of legal moves.
     *
     * That is the case with the two kings alone, with one knight or one bishop (and nothing else), and with bishops
     * only if all of them stand on squares of the same color. Two knights against a lone king are not on the list:
     * checkmate is possible, if the defender helps.
     *
     * @param t_position The position.
     *
     * @return True if the position is a dead position.
     */
    [[nodiscard]] bool is_insufficient_material(const Position& t_position);

    /**
     * @brief Counts how often the position of a node has occurred in the line that leads to it, the node included.
     *
     * Two positions are the same if the pieces, the side to move and the castling rights agree. The en passant square
     * is not compared: it only matters in the position right after a two-square pawn move, and that position cannot
     * come back. Only the positions since the last capture or pawn move are looked at, because earlier ones cannot
     * come back.
     *
     * @param t_game The game.
     * @param t_id The id of the node.
     *
     * @return The number of occurrences, at least 1.
     *
     * @throws std::out_of_range If there is no such node.
     */
    [[nodiscard]] int repetition_count(const GameState& t_game, GameState::NodeId t_id);

    /**
     * @brief Looks for a reason why a position is a draw.
     *
     * A checkmate is never a draw (the move that mates on the 75th move wins). The same goes for a stalemate: it is
     * a draw of its own kind, and this function does not report it. If there are several reasons, the ones that end
     * the game at once come before the ones that only allow a claim.
     *
     * @param t_info The position with the legal moves and the check.
     * @param t_repetitions How often the position has occurred, as from repetition_count().
     *
     * @return The reason, or std::nullopt if the game goes on.
     */
    [[nodiscard]] std::optional<DrawReason> find_draw(const PositionInfo& t_info, int t_repetitions);

    //-------------------------------------------------
    // Texts
    //-------------------------------------------------

    /**
     * @brief Tells whether a draw ends the game at once, or a player has to claim it.
     *
     * @param t_reason The reason.
     *
     * @return True if the game is drawn at once.
     */
    [[nodiscard]] bool is_automatic_draw(DrawReason t_reason) noexcept;

    /**
     * @brief Names a reason in words.
     *
     * @param t_reason The reason.
     *
     * @return For example "insufficient material" or "threefold repetition".
     */
    [[nodiscard]] std::string_view describe_draw(DrawReason t_reason) noexcept;
} // namespace sg::chess::core
