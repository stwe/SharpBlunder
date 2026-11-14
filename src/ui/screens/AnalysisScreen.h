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

#include <string>
#include <string_view>
#include "core/GameState.h"
#include "core/RulesProvider.h"
#include "ui/BoardView.h"
#include "ui/MoveInput.h"
#include "ui/PieceSet.h"
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // AnalysisScreen
    //-------------------------------------------------

    /**
     * @brief The start page: a free board for analysis. There is nothing to learn here.
     *
     * Pieces are moved with the mouse. The played moves are listed on the right, and the user can
     * step through them with the buttons or the arrow keys.
     */
    class AnalysisScreen final : public Screen
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the page with the board in the start position.
         *
         * @param t_pieces The piece images. Not owned: they must outlive this screen.
         * @param t_rules The source of the rules. Not owned: it must outlive this screen.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules);

        AnalysisScreen(const AnalysisScreen& t_other) = delete;
        AnalysisScreen(AnalysisScreen&& t_other) noexcept = delete;
        AnalysisScreen& operator=(const AnalysisScreen& t_other) = delete;
        AnalysisScreen& operator=(AnalysisScreen&& t_other) noexcept = delete;

        ~AnalysisScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        /**
         * @brief Returns the name shown in the navigation bar.
         *
         * @return The title, "Analysis".
         */
        [[nodiscard]] std::string_view Title() const noexcept override;

        /**
         * @brief Handles the mouse input on the board and the keys: arrow keys, Home and End step through the game, F
         * flips the board.
         *
         * @param t_event The event.
         */
        void HandleEvent(const sf::Event& t_event) override;

        /**
         * @brief Draws the board with the pieces and the highlights.
         *
         * @param t_target The surface to draw on.
         */
        void Draw(sf::RenderTarget& t_target) override;

        /**
         * @brief Builds the side panel with Dear ImGui: the status line, the buttons and the list of moves.
         */
        void DrawGui() override;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Collects what the board shows on top of the position: last move, check, selection, dragged piece.
         *
         * @return The overlay for this frame.
         */
        [[nodiscard]] BoardOverlay BuildOverlay() const;

        /**
         * @brief Describes the situation on the board in one line, e.g. "White to move" or "Checkmate".
         *
         * @return The text.
         */
        [[nodiscard]] std::string StatusText() const;

        /**
         * @brief Called after the position on the board has changed by navigation or by a move.
         */
        void OnPositionChanged();

        /**
         * @brief Builds the buttons of the side panel: step through the game, flip the board, start a new game.
         */
        void DrawControls();

        /**
         * @brief Builds the table with the moves of the game. A click on a move jumps to that position.
         */
        void DrawMoveList();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // m_input refers to m_boardView, so m_boardView must be declared first.

        core::GameState m_game;          ///< The moves played on the board
        BoardView m_boardView;           ///< Draws the board
        MoveInput m_input;               ///< Turns mouse input into moves
        bool m_scrollToCurrent{ true };  ///< The move list scrolls to the current move in the next frame
    };
}
