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
#include <unordered_map>
#include "core/RulesProvider.h"
#include "engine/Engine.h"

namespace sg::chess::engine
{
    //-------------------------------------------------
    // StockfishRules
    //-------------------------------------------------

    /**
     * @brief Answers the questions about the rules by asking Stockfish.
     *
     * - "go perft 1" makes Stockfish list all legal moves of a position.
     * - "d" makes it print the FEN of the position (after an optional move) and the pieces giving check.
     *
     * Answers are cached, because the same positions are asked for again and again while moving through a game.
     */
    class StockfishRules final : public core::RulesProvider
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the rules from a running engine.
         *
         * @param t_engine The engine to ask. It must outlive this object and must not be used elsewhere meanwhile.
         */
        explicit StockfishRules(Engine& t_engine);

        StockfishRules(const StockfishRules& t_other) = delete;
        StockfishRules(StockfishRules&& t_other) noexcept = delete;
        StockfishRules& operator=(const StockfishRules& t_other) = delete;
        StockfishRules& operator=(StockfishRules&& t_other) noexcept = delete;

        ~StockfishRules() noexcept override = default;

        //-------------------------------------------------
        // RulesProvider
        //-------------------------------------------------

        /**
         * @brief Describes a position by asking Stockfish: its legal moves and whether the king is in check.
         *
         * @param t_position The position.
         *
         * @return The position with its legal moves and check state.
         *
         * @throws std::runtime_error If Stockfish answers in an unexpected way or not at all.
         */
        [[nodiscard]] core::PositionInfo Describe(const core::Position& t_position) override;

        /**
         * @brief Plays a move and describes the resulting position by asking Stockfish.
         *
         * The move must be legal in the given position.
         *
         * @param t_position The position before the move.
         * @param t_move The move.
         *
         * @return The position after the move with its legal moves and check state.
         *
         * @throws std::runtime_error If Stockfish answers in an unexpected way or not at all.
         */
        [[nodiscard]] core::PositionInfo Play(const core::Position& t_position, const core::Move& t_move) override;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Asks Stockfish about a position, optionally after a move.
         *
         * @param t_fen The position as FEN.
         * @param t_move The move to play first, or std::nullopt.
         *
         * @return The (resulting) position with its legal moves and check state.
         *
         * @throws std::runtime_error If Stockfish answers in an unexpected way or not at all.
         */
        [[nodiscard]] core::PositionInfo Query(const std::string& t_fen, const std::optional<core::Move>& t_move);

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        Engine& m_engine;                                             ///< The engine (not owned)
        std::unordered_map<std::string, core::PositionInfo> m_cache;  ///< Key: "FEN|move"
    };
}
