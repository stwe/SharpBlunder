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

#include <SFML/System/Time.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "core/Move.h"
#include "core/MoveQuality.h"
#include "core/Piece.h"
#include "core/PositionInfo.h"
#include "core/RulesProvider.h"
#include "core/Score.h"
#include "engine/Analyzer.h"
#include "ui/PieceSet.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // EnginePanel
    //-------------------------------------------------

    /**
     * @brief The part of the side panel that shows what the engine thinks: the recommended moves.
     *
     * Every recommendation is one row: a colored dot (how good the move is), the score, and the move with
     * the icon of the piece. A click on the row plays the move, a magnifying glass at its end opens the whole line.
     * While the mouse is on a row, the board shows the arrow of that move. Above the rows, one line says in
     * words who is better. A check box switches the analysis off, because it uses the processor all the time.
     *
     * The panel does not know the game. The screen tells it the current position and plays the move that was
     * clicked.
     */
    class EnginePanel
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the panel. The analysis is on, but only runs while the page is shown, see OnEnter().
         *
         * @param t_pieces The piece images for the icons. Not owned: they must outlive this panel.
         * @param t_rules The source of the rules, needed to write the lines in SAN. Not owned.
         * @param t_analyzer The live analysis. Not owned: it must outlive this panel.
         */
        EnginePanel(const PieceSet& t_pieces, core::RulesProvider& t_rules, engine::Analyzer& t_analyzer);

        EnginePanel(const EnginePanel& t_other) = delete;
        EnginePanel(EnginePanel&& t_other) noexcept = delete;
        EnginePanel& operator=(const EnginePanel& t_other) = delete;
        EnginePanel& operator=(EnginePanel&& t_other) noexcept = delete;

        ~EnginePanel() noexcept = default;

        //-------------------------------------------------
        // Control
        //-------------------------------------------------

        /**
         * @brief Called when the page is shown: starts the analysis again if it is switched on.
         */
        void OnEnter();

        /**
         * @brief Called when another page takes over: stops the analysis, so the engine does not use the processor
         * for a page that is not shown.
         */
        void OnExit();

        /**
         * @brief Tells the panel that the position has changed. The old lines are dropped and the analysis
         * starts again for the new position.
         *
         * @param t_info The new position.
         */
        void PositionChanged(const core::PositionInfo& t_info);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Tells whether the engine works for the panel: the analysis is switched on and the engine is alive.
         *
         * @return True if lines can be expected.
         */
        [[nodiscard]] bool IsActive() const { return m_analysisOn && m_analyzer.IsAlive(); }

        /**
         * @brief Returns the score of the best line, seen from White. It stays until a newer one arrives,
         * also when the position has changed meanwhile. It is empty if nothing was found yet or the analysis is off.
         *
         * @return The score.
         */
        [[nodiscard]] const std::optional<core::Score>& BestScore() const noexcept { return m_bestScore; }

        /**
         * @brief Returns the move that the board should show as an arrow: the move of the row under the mouse,
         * or else the best move.
         *
         * @return The move, or std::nullopt if there is none. It belongs to the position of the last refresh,
         * so the caller must check that it is legal.
         */
        [[nodiscard]] std::optional<core::Move> ArrowMove() const { return m_hoveredMove ? m_hoveredMove : m_bestMove; }

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Takes over the lines of the analysis a few times per second. Does nothing if the analysis is off.
         *
         * @param t_dt The time since the previous frame.
         * @param t_info The current position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        void Update(sf::Time t_dt, const core::PositionInfo& t_info);

        /**
         * @brief Builds the check box and the rows with Dear ImGui. Call it while a GUI frame is being built.
         *
         * The panel is always at least as tall as with all rows shown, also while the engine is still thinking or
         * the analysis is switched off. That way the parts below it do not move. An opened line makes it taller.
         *
         * @param t_info The current position.
         *
         * @return The move of the row that was clicked, or std::nullopt. The caller plays it.
         */
        [[nodiscard]] std::optional<core::Move> Draw(const core::PositionInfo& t_info);

    private:
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
         * @brief Builds the check box and the rows, without the padding to the minimum height.
         *
         * @param t_info The current position.
         *
         * @return The move of the row that was clicked, or std::nullopt.
         */
        [[nodiscard]] std::optional<core::Move> DrawContent(const core::PositionInfo& t_info);

        /**
         * @brief Calculates the height of the panel with all rows shown but no line opened.
         *
         * @return The height in pixels.
         */
        [[nodiscard]] float MinimumHeight() const;

        /**
         * @brief Takes the lines found so far from the analyzer and prepares them for the display.
         *
         * Does nothing as long as the analyzer has no line for the current position.
         *
         * @param t_info The current position.
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        void Refresh(const core::PositionInfo& t_info);

        /**
         * @brief Writes the first moves of an analysis line in SAN, with move numbers.
         *
         * The moves are played on the position with the rules provider, because SAN needs the
         * position before and after each move. Results are cached.
         *
         * @param t_info The position the line starts at.
         * @param t_line The line.
         *
         * @return The moves, e.g. "1." knight "f3", "d5", "2." "d4".
         *
         * @throws std::runtime_error If the rules cannot be queried.
         */
        [[nodiscard]] SanLine ToSan(const core::PositionInfo& t_info, const engine::AnalysisLine& t_line);

        /**
         * @brief Draws the moves of a line one after another, with the icons of the pieces, and breaks the line where
         * the panel ends.
         *
         * @param t_moves The moves.
         */
        void DrawVariation(const std::vector<SanMove>& t_moves) const;

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        const PieceSet& m_pieces;      ///< The piece images for the icons (not owned)
        core::RulesProvider& m_rules;  ///< The source of the rules (not owned)
        engine::Analyzer& m_analyzer;  ///< The live analysis (not owned)

        bool m_analysisOn{ true };                              ///< The check box: analyze the position
        float m_sinceRefresh{ 0.0f };                           ///< Seconds since the lines were taken over
        std::vector<DisplayLine> m_display;                     ///< The lines shown in the panel, best first
        std::string m_assessment;                               ///< The best line in words, e.g. "Equal"
        std::unordered_set<std::string> m_openLines;            ///< First moves (UCI) of the lines shown in full
        std::optional<core::Move> m_bestMove;                   ///< The move of the best line (an arrow)
        std::optional<core::Move> m_hoveredMove;                ///< The move of the row under the mouse (an arrow)
        std::unordered_map<std::string, SanLine> m_variations;  ///< Cache for ToSan(): "FEN|moves"
        std::optional<core::Score> m_bestScore;                 ///< The score of the best line, from White's view
    };
}
