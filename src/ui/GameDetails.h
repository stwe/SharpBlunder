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
#include "core/Pgn.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // GameDetails
    //-------------------------------------------------

    /**
     * @brief The window in which the details of a game are edited: the players and their ratings, the result,
     * the event, the place, the round, the date, the time control and the way the game ended.
     *
     * The details are the tags of the PGN format (White, WhiteElo, Black, BlackElo, Result, Event, Site, Round, Date,
     * TimeControl, Termination), so they are saved with the game and read back from it. Tags that the window does
     * not know stay as they are.
     *
     * The date and the time control are typed the way people write them ("24.11.2026", "15+10") and are saved the
     * way PGN wants them ("2026.11.24", "900+10"). The opening (ECO, Opening, Variation) is shown but not edited: it
     * follows from the moves.
     */
    class GameDetails
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        GameDetails() = default;

        GameDetails(const GameDetails& t_other) = delete;
        GameDetails(GameDetails&& t_other) noexcept = delete;
        GameDetails& operator=(const GameDetails& t_other) = delete;
        GameDetails& operator=(GameDetails&& t_other) noexcept = delete;

        ~GameDetails() noexcept = default;

        //-------------------------------------------------
        // Control
        //-------------------------------------------------

        /**
         * @brief Opens the window. It starts with the current details. The window appears in the next Draw().
         *
         * @param t_headers The details of the game.
         */
        void Open(const core::PgnHeaders& t_headers);

        //-------------------------------------------------
        // Frame steps
        //-------------------------------------------------

        /**
         * @brief Builds the window with Dear ImGui if it is open. Call it while a GUI frame is being built, at the
         * same place every frame.
         *
         * @param t_headers The details of the game. They are changed when the user saves.
         *
         * @return True in the frame in which the user saved.
         */
        [[nodiscard]] bool Draw(core::PgnHeaders& t_headers);

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        static constexpr std::size_t TEXT_SIZE{ 128 };  // the longest text of a field, with the end mark
        static constexpr std::size_t RATING_SIZE{ 8 };

        bool m_wantOpen{ false };  ///< The window is to be opened in the next Draw()

        std::array<char, TEXT_SIZE> m_white{};         ///< Name of White
        std::array<char, RATING_SIZE> m_whiteRating{}; ///< Rating of White
        std::array<char, TEXT_SIZE> m_black{};         ///< Name of Black
        std::array<char, RATING_SIZE> m_blackRating{}; ///< Rating of Black
        std::array<char, TEXT_SIZE> m_event{};         ///< The event, e.g. a tournament
        std::array<char, TEXT_SIZE> m_site{};          ///< The place
        std::array<char, RATING_SIZE> m_round{};       ///< The round
        std::array<char, RATING_SIZE * 2> m_date{};    ///< The date as the user types it, e.g. 2026.11.24 or 24.11.2026
        std::array<char, TEXT_SIZE> m_timeControl{};   ///< The time control as the user types it, e.g. "15+10"
        std::array<char, TEXT_SIZE> m_termination{};   ///< How the game ended, e.g. "White won by resignation"
        int m_result{ 0 };                             ///< The chosen result, an index into the list of results
    };
}
