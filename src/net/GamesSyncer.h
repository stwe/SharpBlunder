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
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include "net/ChessComClient.h"

namespace sg::chess::net
{
    //-------------------------------------------------
    // GamesSyncer
    //-------------------------------------------------

    /**
     * @brief Runs ChessComClient::Fetch() in a thread of its own, so that the window does not freeze.
     *
     * The window asks every frame: IsRunning() and Progress() for the progress bar, and TakeResult() to get the
     * games once they are there. Only one fetch runs at a time.
     */
    class GamesSyncer
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the syncer.
         *
         * @param t_client The client that does the work. It must live longer than the syncer. Only the worker
         *        thread uses it while a fetch runs.
         */
        explicit GamesSyncer(ChessComClient& t_client);
        GamesSyncer(const GamesSyncer& t_other) = delete;
        GamesSyncer(GamesSyncer&& t_other) noexcept = delete;
        GamesSyncer& operator=(const GamesSyncer& t_other) = delete;
        GamesSyncer& operator=(GamesSyncer&& t_other) noexcept = delete;

        /**
         * @brief Stops a running fetch and waits for the thread to end.
         */
        ~GamesSyncer() noexcept;

        //-------------------------------------------------
        // Control
        //-------------------------------------------------

        /**
         * @brief Starts a fetch in the background.
         *
         * @param t_user The user name on chess.com.
         * @param t_maxMonths How many months to fetch, counted from the newest. 0 or less: all.
         * @param t_refreshAll True: ask for months that are over again, too (see ChessComClient::Fetch()).
         *
         * @return False if a fetch is running already. Then nothing was started.
         */
        [[nodiscard]] bool Start(const std::string& t_user, int t_maxMonths, bool t_refreshAll = false);

        /**
         * @brief Asks the running fetch to stop. It stops soon. Does nothing if none runs.
         */
        void Cancel();

        //-------------------------------------------------
        // State
        //-------------------------------------------------

        /**
         * @brief Tells whether a fetch is running.
         *
         * @return True from Start() until the worker thread has put its result away.
         */
        [[nodiscard]] bool IsRunning() const noexcept;

        /**
         * @brief Gives the progress of the running fetch.
         *
         * @return A copy of the last progress. Empty values if there was none yet.
         */
        [[nodiscard]] FetchProgress Progress() const;

        /**
         * @brief Hands over the result of the last fetch, once.
         *
         * @return The result if a fetch has ended and nobody has taken the result yet, otherwise std::nullopt.
         */
        [[nodiscard]] std::optional<FetchResult> TakeResult();

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        ChessComClient& m_client; ///< Does the work
        std::thread m_thread; ///< The worker. Joined before the next start and in the destructor
        std::atomic<bool> m_running{ false }; ///< True while the worker works
        std::atomic<bool> m_cancel{ false }; ///< Set by Cancel(), read by the client
        mutable std::mutex m_mutex; ///< Protects m_progress and m_result
        FetchProgress m_progress; ///< The last progress
        std::optional<FetchResult> m_result; ///< The result, until it is taken
    };
} // namespace sg::chess::net
