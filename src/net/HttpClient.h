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
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace sg::chess::net
{
    //-------------------------------------------------
    // HttpResponse
    //-------------------------------------------------

    /**
     * @brief The answer to a GET request.
     */
    struct HttpResponse
    {
        int status{ 0 }; ///< The HTTP status code, or 0 if there was no answer
        std::map<std::string, std::string> headers; ///< The headers of the answer, with names in lower case
        std::string body; ///< The body of the answer
        std::string error; ///< Why there was no answer (network, timeout, cancel). Empty if there was an answer
        bool cancelled{ false }; ///< True if the request was stopped because of the cancel flag

        /**
         * @brief Tells whether the server answered at all, whatever the status code is.
         *
         * @return True if there is an HTTP status.
         */
        [[nodiscard]] bool Answered() const noexcept
        {
            return status != 0;
        }

        /**
         * @brief Returns a header of the answer.
         *
         * @param t_name The name of the header in lower case, e.g. "etag".
         *
         * @return The value, or an empty text if the answer has no such header.
         */
        [[nodiscard]] std::string Header(const std::string& t_name) const
        {
            const auto it{ headers.find(t_name) };

            return it == headers.end() ? std::string{} : it->second;
        }
    };

    //-------------------------------------------------
    // HttpClient
    //-------------------------------------------------

    /**
     * @brief The way the program asks a web server for something.
     *
     * Only the part that is needed is here: GET with headers. The program uses CurlHttpClient. The tests use a fake
     * one that has recorded answers, so they need no network.
     */
    class HttpClient
    {
    public:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        using Header = std::pair<std::string, std::string>; ///< Name and value of a request header

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        HttpClient() = default;
        HttpClient(const HttpClient& t_other) = delete;
        HttpClient(HttpClient&& t_other) noexcept = delete;
        HttpClient& operator=(const HttpClient& t_other) = delete;
        HttpClient& operator=(HttpClient&& t_other) noexcept = delete;
        virtual ~HttpClient() noexcept = default;

        //-------------------------------------------------
        // Request
        //-------------------------------------------------

        /**
         * @brief Asks for a URL. This blocks until the answer is there, so call it from a worker thread.
         *
         * @param t_url The full URL.
         * @param t_headers Headers to send, e.g. User-Agent and If-None-Match.
         * @param t_cancel If it becomes true while the request runs, the request stops. May be null.
         *
         * @return The answer. Problems of the network are not thrown: they are in HttpResponse::error.
         */
        [[nodiscard]] virtual HttpResponse Get(const std::string& t_url, const std::vector<Header>& t_headers,
                                               const std::atomic<bool>* t_cancel) = 0;
    };
} // namespace sg::chess::net
