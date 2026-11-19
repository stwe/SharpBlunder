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

#include "core/GameState.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // NotationPanel
    //-------------------------------------------------

    /**
     * @brief The part of the side panel that shows the moves of the game and steps through them.
     *
     * The panel changes the position of the GameState it is given (a click on a move, a navigation button)
     * and tells the caller whether it did. Everything that follows from a new position, such as the analysis,
     * is the business of the caller.
     */
    class NotationPanel
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        NotationPanel() = default;

        NotationPanel(const NotationPanel& t_other) = delete;
        NotationPanel(NotationPanel&& t_other) noexcept = delete;
        NotationPanel& operator=(const NotationPanel& t_other) = delete;
        NotationPanel& operator=(NotationPanel&& t_other) noexcept = delete;

        ~NotationPanel() noexcept = default;

        //-------------------------------------------------
        // Control
        //-------------------------------------------------

        /**
         * @brief Makes the list of moves scroll to the current move in the next frame.
         */
        void ScrollToCurrent() noexcept { m_scrollToCurrent = true; }

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Builds the buttons that step through the game: to the start, back, forward, to the end.
         * They are centered in the line.
         *
         * Call it while a GUI frame is being built. It belongs at the bottom of the window, below DrawMoveList().
         *
         * @param t_game The game. A click changes its current position.
         *
         * @return True if the position has changed.
         */
        [[nodiscard]] bool DrawNavigation(core::GameState& t_game);

        /**
         * @brief Builds the list of moves. A click on a move jumps to that position.
         *
         * The main line is a table: a row for each move number, a column for White and one for Black. Variations
         * are written below the move they are an alternative to, indented and in parentheses where they are nested.
         * A click with the right mouse button on a move opens a menu: make its line the main line, or delete
         * the move and everything after it.
         *
         * Call it while a GUI frame is being built. The list fills the rest of the window, but leaves room for
         * the navigation buttons below it.
         *
         * @param t_game The game. A click changes its current position, the menu its moves.
         *
         * @return True if the current position has changed.
         */
        [[nodiscard]] bool DrawMoveList(core::GameState& t_game);

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        bool m_scrollToCurrent{ true };  ///< The move list scrolls to the current move in the next frame
    };
}
