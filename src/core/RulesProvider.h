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

#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // RulesProvider
    //-------------------------------------------------

    /**
     * @brief The source of truth for the rules of chess.
     *
     * The program does not know the rules itself. It asks a RulesProvider which moves are legal,
     * what a position looks like after a move and whether the king is in check.
     * Today the answers come from Stockfish (engine::StockfishRules). Because everything else
     * only knows this interface, a move generator of our own could replace it later.
     */
    class RulesProvider
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        RulesProvider() = default;

        RulesProvider(const RulesProvider& t_other) = delete;
        RulesProvider(RulesProvider&& t_other) noexcept = delete;
        RulesProvider& operator=(const RulesProvider& t_other) = delete;
        RulesProvider& operator=(RulesProvider&& t_other) noexcept = delete;

        /**
         * @brief Virtual, because implementations are deleted through a pointer to this base class.
         */
        virtual ~RulesProvider() noexcept = default;

        //-------------------------------------------------
        // Queries
        //-------------------------------------------------

        /**
         * @brief Describes a position: its legal moves and whether the king is in check.
         *
         * @param t_position The position.
         *
         * @return The position with its legal moves and check state.
         *
         * @throws std::runtime_error If the answer cannot be determined.
         */
        [[nodiscard]] virtual PositionInfo Describe(const Position& t_position) = 0;

        /**
         * @brief Plays a move and describes the resulting position.
         *
         * The move must be legal in the given position.
         *
         * @param t_position The position before the move.
         * @param t_move The move.
         *
         * @return The position after the move with its legal moves and check state.
         *
         * @throws std::runtime_error If the answer cannot be determined.
         */
        [[nodiscard]] virtual PositionInfo Play(const Position& t_position, const Move& t_move) = 0;
    };
}
