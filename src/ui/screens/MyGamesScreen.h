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
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "core/GameList.h"
#include "core/GameSummary.h"
#include "core/Settings.h"
#include "net/GamesSyncer.h"
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // MyGamesScreen
    //-------------------------------------------------

    /**
     * @brief The page with the games of the user from chess.com.
     *
     * The user name comes from the settings and can be changed on the page for the session. The games are fetched in
     * the background (GamesSyncer). A button on each row hands the game to the analysis page.
     */
    class MyGamesScreen final : public Screen
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief What the page calls when the user wants to analyze a game. It shows the game on the analysis page.
         */
        using OpenFunction = std::function<void(const core::GameSummary&)>;

        /**
         * @brief What the page calls after the default user name has been changed: saves the settings.
         *
         * Returns the text of the error, or std::nullopt if it worked.
         */
        using ApplyFunction = std::function<std::optional<std::string>()>;

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the page.
         *
         * @param t_settings The settings. The user name on chess.com is the default of the text field. Not owned.
         * @param t_syncer Fetches the games in the background. Not owned.
         * @param t_open Called when the user wants to analyze a game.
         * @param t_apply Called after the user has chosen a new default name.
         */
        MyGamesScreen(core::Settings& t_settings, net::GamesSyncer& t_syncer, OpenFunction t_open,
                      ApplyFunction t_apply);
        MyGamesScreen(const MyGamesScreen& t_other) = delete;
        MyGamesScreen(MyGamesScreen&& t_other) noexcept = delete;
        MyGamesScreen& operator=(const MyGamesScreen& t_other) = delete;
        MyGamesScreen& operator=(MyGamesScreen&& t_other) noexcept = delete;
        ~MyGamesScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        /**
         * @brief Gives the title of the page.
         *
         * @return "My Games" in the language of the user.
         */
        [[nodiscard]] std::string_view Title() const noexcept override;

        /**
         * @brief Starts the first fetch of the session if a user name is set.
         */
        void OnEnter() override;

        /**
         * @brief Takes the result of the background work, and opens a game that the user has chosen.
         *
         * @param t_dt Time since the last frame. Not used.
         */
        void Update(sf::Time t_dt) override;

        /**
         * @brief Draws the page.
         */
        void DrawGui() override;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Starts a fetch for the name in the text field.
         *
         * @param t_refreshAll True: ask for months that are over, too.
         */
        void StartSync(bool t_refreshAll);

        /**
         * @brief Takes the result of the fetch that has ended, and writes the message for the user.
         *
         * @param t_result The result.
         */
        void OnResult(net::FetchResult t_result);

        /**
         * @brief Draws the user name, the buttons and the message.
         */
        void DrawTop();

        /**
         * @brief Draws the tabs for the time classes and the search field.
         *
         * @return True if the games to show have changed.
         */
        bool DrawFilter();

        /**
         * @brief Draws the table of the games.
         */
        void DrawTable();

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        core::Settings& m_settings; ///< The default name is in here (not owned)
        net::GamesSyncer& m_syncer; ///< Fetches in the background (not owned)
        OpenFunction m_open; ///< Shows a game on the analysis page
        ApplyFunction m_apply; ///< Saves the settings
        std::array<char, core::Settings::MAX_USERNAME_LENGTH + 1> m_username{}; ///< The text field for the name
        std::array<char, 64> m_search{}; ///< The text field for the search
        std::vector<core::GameSummary> m_games; ///< All games that were fetched
        std::vector<std::size_t> m_visible; ///< Indexes in m_games of the games that pass the filter
        std::vector<core::SortKey> m_sort; ///< The columns that the table is sorted by, the first is the most important
        core::GameFilter m_filter; ///< What is chosen (tab and search text)
        core::GameFilter m_shownFilter; ///< The filter that m_visible was made with
        bool m_visibleOutdated{ true }; ///< m_visible has to be made again
        std::string m_loadedUser; ///< The user that the games in m_games belong to
        std::string m_message; ///< The line below the buttons
        bool m_messageIsError{ false }; ///< The message is a problem
        int m_months{ 3 }; ///< How many months are fetched, counted from the newest
        bool m_moreMonths{ false }; ///< chess.com has older months than the ones that were fetched
        bool m_firstEnter{ true }; ///< The page has not been shown yet in this session
        std::optional<std::size_t> m_analyze; ///< The index of the game that the user wants to analyze
    };
} // namespace sg::chess::ui
