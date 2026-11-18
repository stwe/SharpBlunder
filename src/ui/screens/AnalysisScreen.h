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
#include <string_view>
#include "core/GameState.h"
#include "core/OpeningBook.h"
#include "core/RulesProvider.h"
#include "engine/Analyzer.h"
#include "ui/BoardView.h"
#include "ui/EnginePanel.h"
#include "ui/EvalBar.h"
#include "ui/MoveInput.h"
#include "ui/NotationPanel.h"
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
     *
     * An engine analyzes the position while the page is shown: the evaluation bar left of the board,
     * the recommendations in the side panel (see EnginePanel) and an arrow for the best move on the board.
     *
     * The name of the opening is shown above the buttons. It belongs to the last position of the game
     * (up to the current one) that is in the opening book, so it stays when the game leaves the book.
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
         * @param t_analyzer The live analysis. Not owned: it must outlive this screen.
         * @param t_openings The names of the openings. Not owned: it must outlive this screen.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules, engine::Analyzer& t_analyzer,
                       const core::OpeningBook& t_openings);

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
         * @brief Called when the page is shown: starts the analysis again if it is switched on.
         */
        void OnEnter() override;

        /**
         * @brief Called when another page takes over: stops the analysis, so the engine does not use the processor
         * for a page that is not shown.
         */
        void OnExit() override;

        /**
         * @brief Handles the mouse input on the board and the keys: arrow keys, Home and End step through the game, F
         * flips the board.
         *
         * @param t_event The event.
         */
        void HandleEvent(const sf::Event& t_event) override;

        /**
         * @brief Moves the evaluation bar and, a few times per second, takes over the lines of the analysis.
         *
         * @param t_dt The time since the previous frame.
         */
        void Update(sf::Time t_dt) override;

        /**
         * @brief Draws the evaluation bar, the board with the pieces and the highlights.
         *
         * @param t_target The surface to draw on.
         */
        void Draw(sf::RenderTarget& t_target) override;

        /**
         * @brief Builds the side panel with Dear ImGui: the status line, the buttons, the analysis and the list of
         * moves. Also writes the score on the evaluation bar and the labels around the board.
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
         * @return The text. It is short: the hint about the 50-move rule is not part of it.
         */
        [[nodiscard]] std::string StatusText() const;

        /**
         * @brief Looks for the opening of the game: from the current position back to the start,
         * the first position that is in the opening book.
         */
        void UpdateOpening();

        /**
         * @brief Called after the position on the board has changed by navigation or by a move.
         */
        void OnPositionChanged();

        /**
         * @brief Builds the top of the side panel: the name of the opening, the status of the game, and the buttons
         * that flip the board and start a new game. A new game asks first if moves have been played.
         */
        void DrawHeader();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // m_input refers to m_boardView, so m_boardView must be declared first.

        core::GameState m_game;          ///< The moves played on the board
        BoardView m_boardView;           ///< Draws the board
        MoveInput m_input;               ///< Turns mouse input into moves
        const core::OpeningBook& m_openings;  ///< The names of the openings (not owned)
        EvalBar m_evalBar;               ///< The evaluation bar left of the board
        EnginePanel m_enginePanel;       ///< The recommended moves in the side panel
        NotationPanel m_notationPanel;   ///< The list of moves and the navigation buttons

        std::optional<core::Opening> m_opening;  ///< The opening of the game so far, if it has a name
    };
}
