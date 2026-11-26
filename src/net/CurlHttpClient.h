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
#include <string>
#include <vector>
#include "net/HttpClient.h"

namespace sg::chess::net
{
    //-------------------------------------------------
    // CurlHttpClient
    //-------------------------------------------------

    /**
     * @brief Asks web servers with libcurl (SFML cannot do HTTPS).
     *
     * Only https:// addresses are allowed, and redirects only lead to https:// too. There are time limits for
     * connecting and for the whole request, and a limit for the size of the answer. Every request has its own curl
     * handle, so the client can be used from several threads.
     */
    class CurlHttpClient final : public HttpClient
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the client and starts libcurl (once for the program).
         *
         * @param t_allowPlainHttp True also allows http:// addresses. Only the tests need this, for a server on the
         *        same computer.
         */
        explicit CurlHttpClient(bool t_allowPlainHttp = false);
        CurlHttpClient(const CurlHttpClient& t_other) = delete;
        CurlHttpClient(CurlHttpClient&& t_other) noexcept = delete;
        CurlHttpClient& operator=(const CurlHttpClient& t_other) = delete;
        CurlHttpClient& operator=(CurlHttpClient&& t_other) noexcept = delete;
        ~CurlHttpClient() noexcept override = default;

        //-------------------------------------------------
        // HttpClient
        //-------------------------------------------------

        /**
         * @brief Asks for a URL. This blocks, so call it from a worker thread.
         *
         * @param t_url The full address.
         * @param t_headers Headers to send.
         * @param t_cancel If it becomes true while the request runs, the request stops. May be null.
         *
         * @return The answer. Problems of the network are in HttpResponse::error: this function does not throw.
         */
        [[nodiscard]] HttpResponse Get(const std::string& t_url, const std::vector<Header>& t_headers,
                                       const std::atomic<bool>* t_cancel) override;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        bool m_allowPlainHttp; ///< Whether http:// is allowed, too
    };
} // namespace sg::chess::net
