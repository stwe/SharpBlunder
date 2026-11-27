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

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include "core/DrawRules.h"
#include "core/GameState.h"
#include "core/GameSummary.h"
#include "core/OpeningBook.h"
#include "core/Pgn.h"
#include "core/RulesProvider.h"
#include "core/Settings.h"
#include "engine/Analyzer.h"
#include "ui/BoardView.h"
#include "ui/EnginePanel.h"
#include "ui/EvalBar.h"
#include "ui/GameDetails.h"
#include "ui/MoveInput.h"
#include "ui/NotationPanel.h"
#include "ui/PgnMenu.h"
#include "ui/PlayerStrip.h"
#include "ui/PieceSet.h"
#include "ui/Screen.h"
#include "ui/TutorPanel.h"

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
         * @param t_settings The settings of the analysis, read once here.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules, engine::Analyzer& t_analyzer,
                       const core::OpeningBook& t_openings, const core::Settings::Analysis& t_settings);

        AnalysisScreen(const AnalysisScreen& t_other) = delete;
        AnalysisScreen(AnalysisScreen&& t_other) noexcept = delete;
        AnalysisScreen& operator=(const AnalysisScreen& t_other) = delete;
        AnalysisScreen& operator=(AnalysisScreen&& t_other) noexcept = delete;

        ~AnalysisScreen() noexcept override = default;

        //-------------------------------------------------
        // Games from the list
        //-------------------------------------------------

        using BackFunction = std::function<void()>; ///< Goes back to the page that the game was opened from

        /**
         * @brief Sets what the button "My Games" does. It is shown for a game that came from the list of games.
         *
         * @param t_back The function. It switches to the list.
         */
        void SetBackFunction(BackFunction t_back);

        /**
         * @brief Asks to show a game of the list. It is shown when the page is drawn the next time.
         *
         * If the game on the board has been changed by the user (own moves, comments), the user is asked before it is
         * replaced. If the game cannot be read, the page says so and the board stays as it was.
         *
         * @param t_game The game. It carries the PGN text.
         */
        void OpenGame(core::GameSummary t_game);

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
         * @param t_draw The reason why the position is a draw, if it is one (see core::find_draw()).
         *
         * @return The text. It is short: the hint that a player may claim a draw is not part of it.
         */
        [[nodiscard]] std::string StatusText(const std::optional<core::DrawReason>& t_draw) const;

        /**
         * @brief Looks for the opening of the game: from the current position back to the start,
         * the first position that is in the opening book.
         */
        void UpdateOpening();

        /**
         * @brief Writes the opening of the game into its tags (ECO, Opening, Variation, SubVariation).
         *
         * The opening of the game is the last named position of the main line. It does not depend on the position
         * that is shown on the board. Tags that were read from a file are only replaced if the book has a name for the
         * game, and tags that this method wrote are removed again if the game no longer has one.
         */
        void UpdateOpeningTags();

        /**
         * @brief Called after the position on the board has changed by navigation or by a move.
         */
        void OnPositionChanged();

        /**
         * @brief Draws the players above and below the board: picture, name, rating and the captured pieces.
         * The side at the bottom of the board is shown below it.
         */
        void DrawPlayers();

        /**
         * @brief Builds the top of the side panel: the name of the opening, the status of the game, and the buttons
         * that flip the board and start a new game. A new game asks first if moves have been played.
         */
        void DrawHeader();

        /**
         * @brief Handles a game that waits to be opened: replaces the game at once, or asks first.
         */
        void DrawOpenQuestion();

        /**
         * @brief Replaces the game on the board with the game that waits to be opened.
         */
        void LoadListedGame();

        /**
         * @brief Tells whether the user has changed the game on the board since it was opened.
         *
         * @return True if the game has moves and is not the same as when it was opened.
         */
        [[nodiscard]] bool IsModified() const;

        /**
         * @brief Draws the line above the game: the way back to the list, and the accuracy.
         */
        void DrawBackBar();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        // m_input refers to m_boardView, so m_boardView must be declared first.

        core::RulesProvider& m_rules; ///< Checks the moves of games that are opened (not owned)
        core::GameState m_game; ///< The moves played on the board
        BoardView m_boardView; ///< Draws the board
        MoveInput m_input; ///< Turns mouse input into moves
        const core::OpeningBook& m_openings; ///< The names of the openings (not owned)
        EvalBar m_evalBar; ///< The evaluation bar left of the board
        EnginePanel m_enginePanel; ///< The recommended moves in the side panel
        NotationPanel m_notationPanel; ///< The list of moves and the navigation buttons
        PlayerStrip m_playerStrip; ///< The players above and below the board
        TutorPanel m_tutor; ///< The box for the comments of the tutor
        PgnMenu m_pgnMenu; ///< Copy, paste, save and open a game as PGN
        GameDetails m_details; ///< The window for the details of the game: players, result, event, ...
        core::PgnHeaders m_headers; ///< The tags of the game, for PGN

        std::optional<core::Opening> m_opening; ///< The opening of the game so far, if it has a name
        std::optional<core::Opening> m_taggedOpening; ///< The opening that UpdateOpeningTags() wrote into the tags
        BackFunction m_back; ///< Goes back to the list of games
        bool m_cameFromList{ false }; ///< The game was opened from the list, so the way back is shown
        bool m_wantBack{ false }; ///< The button was pressed: go back when it is safe
        std::optional<core::GameSummary> m_listed; ///< The game of the list that is on the board
        std::optional<core::GameSummary> m_pending; ///< A game of the list that waits to be opened
        bool m_asking{ false }; ///< The question "replace the game?" is open
        std::string m_cleanText; ///< The game as PGN when it was opened: to see changes
        bool m_cleanPending{ false }; ///< m_cleanText is taken in the next frame (tags are made then)
        std::string m_openError; ///< Why the last game of the list could not be opened
    };
}
