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
#include <cstdint>
#include <optional>
#include <unordered_set>
#include "core/GameState.h"
#include "core/Pgn.h"

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
     *
     * It also shows the comments and lets the user write them. There are comments on single moves (a speech bubble
     * next to the move), on a variation (a speech bubble in the header of the variation) and on the whole game
     * (in the header of the main line). A comment is shown when the mouse rests on its bubble, or in full after
     * a click on the bubble, or all of them at once with a check box.
     */
    class NotationPanel
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief The three things a comment can belong to.
         */
        enum class CommentKind
        {
            MOVE,  ///< A single move
            LINE,  ///< The variation that starts with a move
            GAME   ///< The whole game: the comment on the start position
        };

        /**
         * @brief A comment: what it belongs to, and the node.
         */
        struct CommentRef
        {
            CommentKind kind{ CommentKind::MOVE };  ///< What the comment belongs to
            core::GameState::NodeId id{ 0 };        ///< The node: the move, or the first move of the variation
        };

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

        /**
         * @brief Forgets what was opened in the list. Call it when another game has been loaded.
         */
        void Reset() noexcept;

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
         * @brief Builds the check box that shows all comments in full, if the game has any comment.
         *
         * Call it while a GUI frame is being built, above DrawMoveList().
         *
         * @param t_game The game.
         *
         * @return True if the check box was drawn, so the caller can put something next to it.
         */
        [[nodiscard]] bool DrawCommentToggle(const core::GameState& t_game);

        /**
         * @brief Builds the list of moves. A click on a move jumps to that position.
         *
         * The main line is a table: a row for each move number, a column for White and one for Black. Variations
         * are written below the move they are an alternative to, indented and in parentheses where they are nested.
         * A click with the right mouse button on a move opens a menu: write a comment, make its line the main line,
         * or delete the move and everything after it.
         *
         * The comments are shown with a speech bubble.
         *
         * Call it while a GUI frame is being built. The list fills the rest of the window, but leaves room for
         * the navigation buttons below it.
         *
         * The header of the table shows the names of the players, and a pencil to edit the details of the game.
         *
         * @param t_game The game. A click changes its current position, the menu its moves.
         * @param t_headers The details of the game, for the names of the players.
         *
         * @return True if the current position has changed.
         */
        [[nodiscard]] bool DrawMoveList(core::GameState& t_game, const core::PgnHeaders& t_headers);

        /**
         * @brief Tells whether the pencil in the header was clicked since the last call.
         *
         * @return True once per click.
         */
        [[nodiscard]] bool TakeDetailsRequest() noexcept;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Opens the window in which a comment is written.
         *
         * @param t_game The game. The window starts with the comment that is there.
         * @param t_ref The comment to edit.
         */
        void BeginEdit(const core::GameState& t_game, CommentRef t_ref);

        /**
         * @brief Builds the window in which a comment is written, if it is open. Saving changes the game.
         *
         * @param t_game The game.
         */
        void DrawEditor(core::GameState& t_game);

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // The longest comment that can be written. The text of a comment that is longer is cut off.
        static constexpr std::size_t EDIT_BUFFER_SIZE{ 4096 };

        bool m_detailsRequested{ false };                    ///< The pencil in the header was clicked
        bool m_scrollToCurrent{ true };                      ///< The list scrolls to the current move in the next frame
        bool m_showAllComments{ false };                     ///< All comments are shown in full
        std::unordered_set<std::uint64_t> m_openComments;    ///< The comments opened with a click on their bubble
        CommentRef m_editTarget;                             ///< The comment that is written in the window
        std::array<char, EDIT_BUFFER_SIZE> m_editBuffer{};   ///< The text in the window
    };
}
