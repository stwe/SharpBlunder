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

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "core/Position.h"
#include "engine/Engine.h"
#include "engine/UciInfo.h"

namespace sg::chess::engine
{
    //-------------------------------------------------
    // Analyzer
    //-------------------------------------------------

    /**
     * @brief Analyzes a position for as long as it is shown: the best lines and their scores.
     *
     * The Analyzer owns a second Stockfish process. StockfishRules keeps asking the first one for the rules,
     * so a running analysis never delays the board. The process gets "go infinite", and a thread reads its answers.
     *
     * The main thread calls SetPosition(), SetRunning() and Lines(). Everything else happens inside.
     *
     * When the position changes, the old search is stopped and a new one is started right away. The engine
     * answers a stopped search with one last "bestmove", and it may still print lines of the old search
     * until then. Those are counted and thrown away, so a line is only used if it belongs to the newest search.
     */
    class Analyzer
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Starts a second engine process. It does not analyze until SetPosition() and SetRunning() are called.
         *
         * @param t_executable Name or path of the engine, see Engine.
         * @param t_lineCount The number of lines to calculate (MultiPV), at least 1.
         *
         * @throws std::runtime_error If the engine cannot be started or does not answer.
         */
        Analyzer(const std::string& t_executable, int t_lineCount);

        Analyzer(const Analyzer& t_other) = delete;
        Analyzer(Analyzer&& t_other) noexcept = delete;
        Analyzer& operator=(const Analyzer& t_other) = delete;
        Analyzer& operator=(Analyzer&& t_other) noexcept = delete;

        /**
         * @brief Stops the engine and the reading thread.
         */
        ~Analyzer() noexcept;

        //-------------------------------------------------
        // Control
        //-------------------------------------------------

        /**
         * @brief Sets the position to analyze. The old lines are dropped at once.
         *
         * Does nothing if the position is the one already analyzed.
         *
         * @param t_position The position.
         */
        void SetPosition(const core::Position& t_position);

        /**
         * @brief Switches the analysis on or off. The engine is idle while it is off.
         *
         * @param t_running True to analyze.
         */
        void SetRunning(bool t_running);

        //-------------------------------------------------
        // Query
        //-------------------------------------------------

        /**
         * @brief Tells whether the analysis is switched on.
         *
         * @return True if the engine is asked to analyze.
         */
        [[nodiscard]] bool IsRunning() const;

        /**
         * @brief Tells whether the engine still works.
         *
         * @return False after the engine has ended or could not be written to.
         */
        [[nodiscard]] bool IsAlive() const noexcept { return !m_failed; }

        /**
         * @brief Returns the current lines, best first. This is a copy, so it is safe to keep.
         *
         * The scores are seen from White: positive is good for White, whoever is to move.
         * The list is empty as long as the engine has not found anything for the position.
         *
         * @return The lines.
         */
        [[nodiscard]] std::vector<AnalysisLine> Lines() const;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Stops the running search and starts a new one if the analysis is on and a position is set.
         */
        void Restart();

        /**
         * @brief Sends a command. If that fails, the analyzer is marked as failed instead of throwing.
         *
         * @param t_command The command line.
         */
        void Send(std::string_view t_command);

        /**
         * @brief The body of the reading thread: reads until the engine's output ends.
         */
        void ReadLoop();

        /**
         * @brief Takes over a line of an analysis, unless it belongs to an old search.
         *
         * @param t_line The parsed line.
         */
        void Accept(AnalysisLine t_line);

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        Engine m_engine; ///< The second Stockfish process
        int m_lineCount; ///< The number of lines (MultiPV)
        std::atomic<bool> m_failed{ false }; ///< The engine has ended or cannot be written to

        mutable std::mutex m_mutex; ///< Protects everything below
        std::string m_fen; ///< The position to analyze, empty if there is none yet
        bool m_whiteToMove{ true }; ///< Side to move in m_fen; scores are turned to White with it
        bool m_running{ false }; ///< The analysis is switched on
        bool m_accepting{ false }; ///< Lines of the newest search are stored
        std::size_t m_started{ 0 }; ///< The number of searches started ("go")
        std::size_t m_finished{ 0 }; ///< The number of searches the engine has ended ("bestmove")
        std::vector<AnalysisLine> m_lines; ///< The lines found, sorted by rank

        // Declared last: the thread starts in the constructor and uses the members above.
        std::thread m_reader; ///< Reads the engine's answers
    };
} // namespace sg::chess::engine
