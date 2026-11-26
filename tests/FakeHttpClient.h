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

#include <algorithm>
#include <atomic>
#include <map>
#include <string>
#include <vector>
#include "net/HttpClient.h"

namespace sg::chess::tests
{
    /**
     * @brief A web client without a network: it answers with what the test has put in.
     *
     * Answers for one URL are used one after the other. The last one is repeated. A URL that was not put in gets
     * status 404. Every request is recorded, so a test can look at the URLs and the headers.
     */
    class FakeHttpClient final : public net::HttpClient
    {
    public:
        /**
         * @brief A request that the client has received.
         */
        struct Request
        {
            std::string url; ///< The URL
            std::vector<net::HttpClient::Header> headers; ///< The headers

            /**
             * @brief Returns a header of the request.
             *
             * @param t_name The name of the header (exact spelling).
             *
             * @return The value, or an empty text.
             */
            [[nodiscard]] std::string Value(const std::string& t_name) const
            {
                for (const auto& [name, value] : headers)
                {
                    if (name == t_name)
                    {
                        return value;
                    }
                }

                return {};
            }
        };

        /**
         * @brief Adds an answer for a URL.
         *
         * @param t_url The URL.
         * @param t_response The answer.
         */
        void Add(const std::string& t_url, net::HttpResponse t_response)
        {
            m_answers[t_url].push_back(std::move(t_response));
        }

        /**
         * @brief Adds a normal answer (status 200) with an ETag.
         *
         * @param t_url The URL.
         * @param t_body The body.
         * @param t_etag The ETag. If it is empty, there is no ETag header.
         */
        void AddOk(const std::string& t_url, std::string t_body, const std::string& t_etag = "")
        {
            net::HttpResponse response;
            response.status = 200;
            response.body = std::move(t_body);

            if (!t_etag.empty())
            {
                response.headers["etag"] = t_etag;
            }

            Add(t_url, std::move(response));
        }

        /**
         * @brief Adds an answer that has only a status code (404, 429, 304 ...).
         *
         * @param t_url The URL.
         * @param t_status The status code.
         */
        void AddStatus(const std::string& t_url, const int t_status)
        {
            net::HttpResponse response;
            response.status = t_status;

            Add(t_url, std::move(response));
        }

        /**
         * @brief Adds a network error: no answer at all.
         *
         * @param t_url The URL.
         * @param t_error The text of the error.
         */
        void AddNetworkError(const std::string& t_url, const std::string& t_error)
        {
            net::HttpResponse response;
            response.error = t_error;

            Add(t_url, std::move(response));
        }

        /**
         * @brief Gives the recorded requests.
         *
         * @return All requests, in the order they came.
         */
        [[nodiscard]] const std::vector<Request>& Requests() const noexcept
        {
            return m_requests;
        }

        /**
         * @brief Counts the requests for a URL.
         *
         * @param t_url The URL.
         *
         * @return The number of requests.
         */
        [[nodiscard]] int Count(const std::string& t_url) const
        {
            int count{ 0 };

            for (const Request& request : m_requests)
            {
                count += request.url == t_url ? 1 : 0;
            }

            return count;
        }

        /**
         * @brief Answers a request from the recorded answers.
         *
         * @param t_url The URL.
         * @param t_headers The headers of the request.
         * @param t_cancel Not used by the fake client, except that a set flag gives a cancelled answer.
         *
         * @return The next answer for the URL, or 404.
         */
        [[nodiscard]] net::HttpResponse Get(const std::string& t_url, const std::vector<Header>& t_headers,
                                            const std::atomic<bool>* t_cancel) override
        {
            m_requests.push_back({ t_url, t_headers });

            if (t_cancel != nullptr && t_cancel->load())
            {
                net::HttpResponse response;
                response.cancelled = true;
                response.error = "cancelled";

                return response;
            }

            const auto it{ m_answers.find(t_url) };

            if (it == m_answers.end() || it->second.empty())
            {
                net::HttpResponse response;
                response.status = 404;

                return response;
            }

            auto& list{ it->second };
            const std::size_t index{ std::min(m_used[t_url]++, list.size() - 1) };

            return list[index];
        }

    private:
        std::map<std::string, std::vector<net::HttpResponse>> m_answers; ///< The answers for each URL
        std::map<std::string, std::size_t> m_used; ///< How many answers of a URL were given
        std::vector<Request> m_requests; ///< All requests
    };
} // namespace sg::chess::tests
