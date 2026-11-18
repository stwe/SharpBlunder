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
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "core/GameState.h"
#include "core/Move.h"
#include "core/MoveQuality.h"
#include "core/OpeningBook.h"
#include "core/Piece.h"
#include "core/RulesProvider.h"
#include "engine/Analyzer.h"
#include "ui/BoardView.h"
#include "ui/EvalBar.h"
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
     *
     * An engine analyzes the position while the page is shown: the evaluation bar left of the board,
     * the recommendations in the side panel and an arrow for the best move on the board. A check box switches
     * the analysis off, because it uses the processor all the time.
     *
     * Every recommendation is one row: a colored dot (how good the move is), the score, and the move with
     * the icon of the piece. A click on the row plays the move, a magnifying glass at its end opens the whole line.
     * While the mouse is on a row, the board shows the arrow of that move. Above the rows, one line says in
     * words who is better.
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
        // Types
        //-------------------------------------------------

        /**
         * @brief One move of a line, written for the display.
         */
        struct SanMove
        {
            std::string number;                 ///< "1." before a White move, "1..." before a Black one, or empty
            std::optional<core::Piece> figure;  ///< The piece that moves: shown as an icon instead of a letter
            std::string text;                   ///< The move in SAN, without the piece letter if there is an icon
        };

        /**
         * @brief A line of the analysis written in SAN: the first moves, ready to be shown.
         */
        struct SanLine
        {
            std::vector<SanMove> moves;  ///< The moves, starting at the current position
        };

        /**
         * @brief One line of the analysis, ready to be shown.
         */
        struct DisplayLine
        {
            int depth{ 0 };                ///< Search depth in plies
            std::string score;             ///< The score from White's point of view, e.g. "+0.35"
            std::string description;       ///< The score in words, e.g. "White is slightly better"
            core::Move move;               ///< The first move of the line
            core::MoveQuality quality;     ///< How much weaker the move is than the best one
            std::vector<SanMove> moves;    ///< The first moves of the line; the first one is the move of the row
        };

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
         * @brief Looks for the opening of the game: from the current position back to the start,
         * the first position that is in the opening book.
         */
        void UpdateOpening();

        /**
         * @brief Called after the position on the board has changed by navigation or by a move.
         */
        void OnPositionChanged();

        /**
         * @brief Builds the buttons of the side panel: step through the game, flip the board, start a new game.
         */
        void DrawControls();

        /**
         * @brief Builds the analysis part of the side panel: the check box and one row for each recommended move.
         *
         * A click on a row plays the move, a click on the magnifying glass shows or hides the line.
         */
        void DrawAnalysis();

        /**
         * @brief Takes the lines found so far from the analyzer and shows them: bar, list and arrow.
         *
         * Does nothing as long as the analyzer has no line for the current position, so the bar stays where it is.
         */
        void RefreshAnalysis();

        /**
         * @brief Writes the first moves of an analysis line in SAN, with move numbers.
         *
         * The moves are played on the current position with the rules provider, because SAN needs the
         * position before and after each move. Results are cached.
         *
         * @param t_line The line, starting at the current position.
         *
         * @return The moves, e.g. "1." knight "f3", "d5", "2." "d4".
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        [[nodiscard]] SanLine ToSan(const engine::AnalysisLine& t_line);

        /**
         * @brief Draws the moves of a line one after another, with the icons of the pieces, and breaks the line where
         * the panel ends.
         *
         * @param t_moves The moves.
         */
        void DrawVariation(const std::vector<SanMove>& t_moves) const;

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
        const PieceSet& m_pieces;        ///< The piece images, also used as small icons in the panel (not owned)
        core::RulesProvider& m_rules;    ///< The source of the rules (not owned)
        engine::Analyzer& m_analyzer;    ///< The live analysis (not owned)
        const core::OpeningBook& m_openings;  ///< The names of the openings (not owned)
        EvalBar m_evalBar;               ///< The evaluation bar left of the board
        bool m_scrollToCurrent{ true };  ///< The move list scrolls to the current move in the next frame

        std::optional<core::Opening> m_opening;  ///< The opening of the game so far, if it has a name

        bool m_analysisOn{ true };                              ///< The check box: analyze the position
        float m_sinceRefresh{ 0.0f };                           ///< Seconds since the lines were taken over
        std::vector<DisplayLine> m_display;                     ///< The lines shown in the panel, best first
        std::string m_assessment;                               ///< The best line in words, e.g. "Equal"
        std::unordered_set<std::string> m_openLines;            ///< First moves (UCI) of the lines shown in full
        std::optional<core::Move> m_bestMove;                   ///< The move of the best line (an arrow)
        std::optional<core::Move> m_hoveredMove;                ///< The move of the row under the mouse (an arrow)
        std::unordered_map<std::string, SanLine> m_variations;  ///< Cache for ToSan(): "FEN|moves"
    };
}
