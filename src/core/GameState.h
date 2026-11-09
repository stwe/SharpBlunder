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

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"
#include "core/RulesProvider.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // GameState
    //-------------------------------------------------

    /**
     * @brief A game as a list of moves, with a current position that can be moved back and forth.
     *
     * Entry 0 is the start position, entry n is the position after the n-th move.
     * "Ply" means a single move of one side: White's first move is ply 1, Black's reply is ply 2.
     *
     * The rules (legal moves, check) come from a RulesProvider. The game state itself only stores the results.
     */
    class GameState
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief One position of the game and the move that led to it.
         */
        struct Entry
        {
            PositionInfo info;         ///< The position with its legal moves
            std::optional<Move> move;  ///< The move that led here (empty for the start position)
            std::string san;           ///< That move in SAN (empty for the start position)
        };

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Starts a game from the standard start position.
         *
         * @param t_rules The source of the rules. It must outlive this object.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        explicit GameState(RulesProvider& t_rules);

        /**
         * @brief Starts a game from a given position.
         *
         * @param t_rules The source of the rules. It must outlive this object.
         * @param t_start The start position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        GameState(RulesProvider& t_rules, const Position& t_start);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Returns the current entry.
         *
         * @return The position the board shows, with the move that led to it.
         */
        [[nodiscard]] const Entry& Current() const noexcept { return m_entries[m_current]; }

        /**
         * @brief Returns all entries. Index 0 is the start position.
         *
         * @return A read-only view of the entries.
         */
        [[nodiscard]] std::span<const Entry> Entries() const noexcept { return m_entries; }

        /**
         * @brief Returns the index of the current entry: 0 is the start position.
         *
         * @return The current ply.
         */
        [[nodiscard]] std::size_t CurrentPly() const noexcept { return m_current; }

        /**
         * @brief Returns the number of moves in the game, including those after the current position.
         *
         * @return The number of plies.
         */
        [[nodiscard]] std::size_t PlyCount() const noexcept { return m_entries.size() - 1; }

        [[nodiscard]] bool CanGoBack() const noexcept { return m_current > 0; }
        [[nodiscard]] bool CanGoForward() const noexcept { return m_current < PlyCount(); }

        //-------------------------------------------------
        // Navigation
        //-------------------------------------------------

        void GoBack() noexcept;
        void GoForward() noexcept;
        void GoToStart() noexcept;
        void GoToEnd() noexcept;

        /**
         * @brief Jumps to a position of the game.
         *
         * @param t_ply The ply to jump to. Values beyond the last move jump to the end.
         */
        void GoTo(std::size_t t_ply) noexcept;

        //-------------------------------------------------
        // Changes
        //-------------------------------------------------

        /**
         * @brief Plays a move in the current position.
         *
         * If the move is the one that already follows in the game, the game just moves forward.
         * Otherwise all moves after the current position are discarded and the new move is appended.
         *
         * @param t_move The move.
         *
         * @return True if the move was played, false if it is not legal in the current position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        bool Play(const Move& t_move);

        /**
         * @brief Removes all moves and returns to the start position.
         */
        void Reset() noexcept;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        RulesProvider& m_rules;        ///< The source of the rules (not owned)
        std::vector<Entry> m_entries;  ///< Index 0 = start position
        std::size_t m_current{ 0 };    ///< Index of the current entry
    };
}
