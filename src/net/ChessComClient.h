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
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include "core/GameSummary.h"
#include "net/HttpClient.h"

namespace sg::chess::net
{
    //-------------------------------------------------
    // Types
    //-------------------------------------------------

    /**
     * @brief What went wrong when the games were fetched.
     */
    enum class FetchError : std::uint8_t
    {
        NONE, ///< Nothing
        INVALID_USERNAME, ///< The name cannot be a name of chess.com. No request was made
        USER_NOT_FOUND, ///< chess.com does not know the name
        RATE_LIMITED, ///< chess.com said "too many requests" and still did after waiting
        NETWORK, ///< No answer: no connection, timeout
        SERVER, ///< chess.com answered with an error
        BAD_RESPONSE, ///< The answer is not what was expected
        CANCELLED ///< The user stopped it
    };

    /**
     * @brief How far the fetching is.
     */
    struct FetchProgress
    {
        int done{ 0 }; ///< Months that are finished
        int total{ 0 }; ///< Months that will be fetched
        std::string month; ///< The month that is being fetched now, e.g. "2026-11"
    };

    /**
     * @brief The result of ChessComClient::Fetch().
     *
     * Games can be there even if there is an error: what is in the cache is used if the network fails.
     */
    struct FetchResult
    {
        std::vector<core::GameSummary> games; ///< The games, the newest first
        FetchError error{ FetchError::NONE }; ///< What went wrong, if something did
        std::string message; ///< Details of the error in English, for the log. Empty if no error
        int monthsFromCache{ 0 }; ///< Months that were not downloaded because the cache was good
        int monthsDownloaded{ 0 }; ///< Months that were downloaded
        int skippedVariants{ 0 }; ///< Games of variants that the program cannot show
        int skippedBroken{ 0 }; ///< Games that could not be read
        bool moreMonthsAvailable{ false }; ///< Whether chess.com has older months than the ones fetched
    };

    /**
     * @brief The settings of the client.
     */
    struct ClientOptions
    {
        /**
         * @brief Waits. The first argument is the time in milliseconds. The second is the cancel flag (may be null).
         *        The default waits in short steps, so that cancelling does not take long. Tests use a function that
         *        does not wait.
         */
        std::function<void(int, const std::atomic<bool>*)> sleep;

        std::function<std::time_t()> now; ///< The current time. The default is the clock of the system
        std::string userAgent; ///< Sent with every request. chess.com asks for contact data in it
        int maxRetries{ 2 }; ///< How often a request is repeated after "429 too many requests"
    };

    //-------------------------------------------------
    // ChessComClient
    //-------------------------------------------------

    /**
     * @brief Fetches the games of a user from the published-data API of chess.com.
     *
     * Only documented endpoints are used: the list of archives and the games of a month. The requests are made one
     * after the other. The answers are kept in a cache folder, so that a month that is over is downloaded only once.
     * Newer months are asked for again with the ETag of the last answer: if nothing has changed, chess.com sends no
     * body. If there is no network, the cache is used.
     */
    class ChessComClient
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        using ProgressFunction = std::function<void(const FetchProgress&)>; ///< Called after every month

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the client.
         *
         * @param t_http The way to ask the server. It must live longer than the client.
         * @param t_cacheRoot The folder for the cache. There is a folder for each user in it.
         * @param t_options Waiting, time and the User-Agent.
         */
        ChessComClient(HttpClient& t_http, std::filesystem::path t_cacheRoot, ClientOptions t_options);
        ChessComClient(const ChessComClient& t_other) = delete;
        ChessComClient(ChessComClient&& t_other) noexcept = delete;
        ChessComClient& operator=(const ChessComClient& t_other) = delete;
        ChessComClient& operator=(ChessComClient&& t_other) noexcept = delete;
        ~ChessComClient() noexcept = default;

        //-------------------------------------------------
        // Fetching
        //-------------------------------------------------

        /**
         * @brief Fetches the games of a user. This blocks, so call it from a worker thread.
         *
         * @param t_user The name of the user on chess.com. Upper and lower case do not matter.
         * @param t_maxMonths How many months to fetch, counted from the newest. 0 or less means all of them.
         * @param t_progress Called after every month. May be empty.
         * @param t_cancel If it becomes true, the work stops soon. May be null.
         * @param t_refreshAll True: ask for every month again, also for those that are over. The ETag is still sent,
         *        so what has not changed costs no download.
         *
         * @return The games, and the error if there was one. This function does not throw.
         */
        [[nodiscard]] FetchResult Fetch(const std::string& t_user, int t_maxMonths, const ProgressFunction& t_progress,
                                        const std::atomic<bool>* t_cancel, bool t_refreshAll = false);

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        HttpClient& m_http; ///< Asks the server
        std::filesystem::path m_cacheRoot; ///< The folder of the cache
        ClientOptions m_options; ///< Waiting, time and the User-Agent
    };
} // namespace sg::chess::net
