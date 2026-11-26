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

#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <mutex>
#include <string_view>
#include "net/CurlHttpClient.h"

namespace
{
    // Time limits in seconds: for the connection, and for everything.
    constexpr long CONNECT_TIMEOUT_SECONDS{ 15 };
    constexpr long TOTAL_TIMEOUT_SECONDS{ 120 };

    // A month of games is a few megabytes. Anything much bigger is not what we asked for.
    constexpr std::size_t MAX_BODY_BYTES{ 128UL * 1024UL * 1024UL };

    // Redirects are followed a few times. A longer chain is an error.
    constexpr long MAX_REDIRECTS{ 3 };

    // Everything that the callbacks of curl need to know.
    struct Transfer
    {
        sg::chess::net::HttpResponse* response{ nullptr };
        const std::atomic<bool>* cancel{ nullptr };
        bool tooBig{ false };
    };

    std::string Lower(std::string t_text)
    {
        std::ranges::transform(t_text, t_text.begin(),
                               [](const unsigned char t_c) { return static_cast<char>(std::tolower(t_c)); });

        return t_text;
    }

    std::string_view Trim(std::string_view t_text)
    {
        while (!t_text.empty() && std::isspace(static_cast<unsigned char>(t_text.front())) != 0)
        {
            t_text.remove_prefix(1);
        }

        while (!t_text.empty() && std::isspace(static_cast<unsigned char>(t_text.back())) != 0)
        {
            t_text.remove_suffix(1);
        }

        return t_text;
    }

    std::size_t OnBody(char* t_data, const std::size_t t_size, const std::size_t t_count, void* t_user)
    {
        auto* transfer{ static_cast<Transfer*>(t_user) };
        const std::size_t bytes{ t_size * t_count };

        if (transfer->response->body.size() + bytes > MAX_BODY_BYTES)
        {
            transfer->tooBig = true;

            return 0;  // Returning less than was given stops the transfer.
        }

        transfer->response->body.append(t_data, bytes);

        return bytes;
    }

    std::size_t OnHeader(char* t_data, const std::size_t t_size, const std::size_t t_count, void* t_user)
    {
        auto* transfer{ static_cast<Transfer*>(t_user) };
        const std::size_t bytes{ t_size * t_count };
        const std::string_view line{ t_data, bytes };

        // A new answer starts (after a redirect): the headers of the one before do not count any more.
        if (line.starts_with("HTTP/"))
        {
            transfer->response->headers.clear();

            return bytes;
        }

        if (const std::size_t colon{ line.find(':') }; colon != std::string_view::npos)
        {
            const std::string name{ Lower(std::string{ Trim(line.substr(0, colon)) }) };
            transfer->response->headers[name] = std::string{ Trim(line.substr(colon + 1)) };
        }

        return bytes;
    }

    // Called by curl again and again. A result other than 0 stops the transfer.
    int OnProgress(void* t_user, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
    {
        const auto* transfer{ static_cast<const Transfer*>(t_user) };

        return transfer->cancel != nullptr && transfer->cancel->load() ? 1 : 0;
    }

    // Starts libcurl once. It is not allowed to do this from several threads at the same time.
    void InitCurl()
    {
        static std::once_flag once;
        std::call_once(once, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
    }

    // curl_easy_setopt wants the protocols as a text from version 7.85 on. Before, as numbers.
    void RestrictProtocols(CURL* t_curl, const bool t_allowPlainHttp)
    {
#if LIBCURL_VERSION_NUM >= 0x075500
        const char* protocols{ t_allowPlainHttp ? "https,http" : "https" };
        curl_easy_setopt(t_curl, CURLOPT_PROTOCOLS_STR, protocols);
        curl_easy_setopt(t_curl, CURLOPT_REDIR_PROTOCOLS_STR, protocols);
#else
        const long protocols{ t_allowPlainHttp ? (CURLPROTO_HTTPS | CURLPROTO_HTTP) : CURLPROTO_HTTPS };
        curl_easy_setopt(t_curl, CURLOPT_PROTOCOLS, protocols);
        curl_easy_setopt(t_curl, CURLOPT_REDIR_PROTOCOLS, protocols);
#endif
    }
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::net::CurlHttpClient::CurlHttpClient(const bool t_allowPlainHttp)
    : m_allowPlainHttp(t_allowPlainHttp)
{
    InitCurl();
}

//-------------------------------------------------
// HttpClient
//-------------------------------------------------

sg::chess::net::HttpResponse sg::chess::net::CurlHttpClient::Get(const std::string& t_url,
                                                                 const std::vector<Header>& t_headers,
                                                                 const std::atomic<bool>* t_cancel)
{
    HttpResponse response;

    if (!t_url.starts_with("https://") && !(m_allowPlainHttp && t_url.starts_with("http://")))
    {
        response.error = "Only https:// addresses are allowed";

        return response;
    }

    CURL* curl{ curl_easy_init() };

    if (curl == nullptr)
    {
        response.error = "libcurl could not be started";

        return response;
    }

    curl_slist* list{ nullptr };

    for (const auto& [name, value] : t_headers)
    {
        // A line break in a header would add more headers.
        if (name.find_first_of("\r\n") == std::string::npos && value.find_first_of("\r\n") == std::string::npos)
        {
            list = curl_slist_append(list, (name + ": " + value).c_str());
        }
    }

    Transfer transfer;
    transfer.response = &response;
    transfer.cancel = t_cancel;

    curl_easy_setopt(curl, CURLOPT_URL, t_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnBody);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, OnHeader);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, OnProgress);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &transfer);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, MAX_REDIRECTS);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, TOTAL_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);  // Needed in programs with threads: no signals for timeouts.
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // All encodings that this curl can decode (gzip ...).
    RestrictProtocols(curl, m_allowPlainHttp);

    char errorText[CURL_ERROR_SIZE]{};
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);

    const CURLcode code{ curl_easy_perform(curl) };

    if (code == CURLE_OK)
    {
        long status{ 0 };
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        response.status = static_cast<int>(status);
    }
    else
    {
        // No usable answer: whatever was received before is dropped.
        response.status = 0;
        response.body.clear();
        response.headers.clear();

        if (code == CURLE_ABORTED_BY_CALLBACK && t_cancel != nullptr && t_cancel->load())
        {
            response.cancelled = true;
            response.error = "cancelled";
        }
        else if (transfer.tooBig)
        {
            response.error = "The answer is too big";
        }
        else
        {
            response.error = errorText[0] != '\0' ? std::string{ errorText } : std::string{ curl_easy_strerror(code) };
        }
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    return response;
}
