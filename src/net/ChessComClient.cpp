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

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <optional>
#include <system_error>
#include <thread>
#include <utility>
#include "core/ChessComParser.h"
#include "core/GameList.h"
#include "net/ChessComClient.h"

namespace
{
 // Json objects are made with "=" and not with braces: braces around a single value make an array.
    using Json = nlohmann::ordered_json;
    namespace fs = std::filesystem;
    namespace net = sg::chess::net;
    namespace core = sg::chess::core;

 // The address of the published-data API. A user's games are at <ROOT><name>/games/...
    constexpr const char* API_ROOT{ "https://api.chess.com/pub/player/" };

 // How long to wait after "429 too many requests" if the answer does not say: seconds, and the most we wait.
    constexpr int DEFAULT_RETRY_SECONDS{ 5 };
    constexpr int MAX_RETRY_SECONDS{ 60 };

 // The waiting is cut into short steps, so that "cancel" works at once.
    constexpr int SLEEP_STEP_MS{ 100 };

 // A month is final when it is at least this many months in the past: no game can be added to it any more.
 // The two latest months are asked for again every time, but only with the ETag, so this costs very little.
    constexpr int FINAL_AFTER_MONTHS{ 2 };

    constexpr int MONTHS_PER_YEAR{ 12 };
    constexpr int HTTP_OK{ 200 };
    constexpr int HTTP_NOT_MODIFIED{ 304 };
    constexpr int HTTP_NOT_FOUND{ 404 };
    constexpr int HTTP_GONE{ 410 };
    constexpr int HTTP_TOO_MANY_REQUESTS{ 429 };

    constexpr int JSON_INDENT{ 0 };

    //-------------------------------------------------
 // Cache
    //-------------------------------------------------

 // What is known about an answer that is in the cache.
    struct CacheMeta
    {
        std::string etag; // To ask "has it changed?" the next time
        std::string lastModified; // The same, if there is no ETag
        bool final{ false }; // True if it was fetched after the month was over for good
    };

    struct Cached
    {
        std::string body;
        CacheMeta meta;
    };

    std::optional<std::string> ReadFile(const fs::path& t_path)
    {
        std::error_code ec;

        if (!fs::is_regular_file(t_path, ec))
        {
            return std::nullopt;
        }

        std::ifstream file{ t_path, std::ios::binary };

        if (!file)
        {
            return std::nullopt;
        }

        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }

 // Writes to "<n>.tmp" and renames, so that a crash leaves no half-written file. The cache is only a cache:
 // if writing fails, the program goes on without it.
    void WriteFile(const fs::path& t_path, const std::string& t_text)
    {
        std::error_code ec;
        fs::create_directories(t_path.parent_path(), ec);

        fs::path temporary{ t_path };
        temporary += ".tmp";

        {
            std::ofstream file{ temporary, std::ios::binary | std::ios::trunc };
            file << t_text;
            file.flush();

            if (!file)
            {
                fs::remove(temporary, ec);
                return;
            }
        }

        fs::rename(temporary, t_path, ec);

        if (ec)
        {
            fs::remove(temporary, ec);
        }
    }

    std::optional<Cached> LoadCache(const fs::path& t_directory, const std::string& t_name)
    {
        auto body{ ReadFile(t_directory / (t_name + ".json")) };

        if (!body)
        {
            return std::nullopt;
        }

        Cached cached;
        cached.body = std::move(*body);

        if (const auto text{ ReadFile(t_directory / (t_name + ".meta.json")) })
        {
            const Json meta = Json::parse(*text, nullptr, false);

            if (meta.is_object())
            {
                cached.meta.etag = meta.value("etag", std::string{});
                cached.meta.lastModified = meta.value("lastModified", std::string{});
                cached.meta.final = meta.value("final", false);
            }
        }

        return cached;
    }

    void SaveMeta(const fs::path& t_directory, const std::string& t_name, const CacheMeta& t_meta)
    {
        Json meta = Json::object();
        meta["etag"] = t_meta.etag;
        meta["lastModified"] = t_meta.lastModified;
        meta["final"] = t_meta.final;

        WriteFile(t_directory / (t_name + ".meta.json"), meta.dump(JSON_INDENT) + "\n");
    }

    void SaveCache(const fs::path& t_directory, const std::string& t_name, const std::string& t_body,
                   const CacheMeta& t_meta)
    {
        WriteFile(t_directory / (t_name + ".json"), t_body);
        SaveMeta(t_directory, t_name, t_meta);
    }

    //-------------------------------------------------
 // Months
    //-------------------------------------------------

 // A month as one number: months since the year 0. Two months can be compared and counted with it.
    int MonthIndex(const int t_year, const int t_month)
    {
        return t_year * MONTHS_PER_YEAR + (t_month - 1);
    }

 // "2026/11" as the index of the month, or nothing if the text is not of this form.
    std::optional<int> MonthIndexOf(const std::string& t_tail)
    {
        const auto isDigit = [](const char t_c) { return t_c >= '0' && t_c <= '9'; };

        if (t_tail.size() != 7 || t_tail[4] != '/' || !std::ranges::all_of(t_tail.substr(0, 4), isDigit)
            || !std::ranges::all_of(t_tail.substr(5), isDigit))
        {
            return std::nullopt;
        }

        const int year{ std::stoi(t_tail.substr(0, 4)) };
        const int month{ std::stoi(t_tail.substr(5)) };

        return month >= 1 && month <= MONTHS_PER_YEAR ? std::optional<int>{ MonthIndex(year, month) } : std::nullopt;
    }

 // The label of a month for the cache files and for the progress: "2026-11".
    std::string MonthLabel(const int t_index)
    {
        const int year{ t_index / MONTHS_PER_YEAR };
        const int month{ t_index % MONTHS_PER_YEAR + 1 };

        return std::to_string(year) + (month < 10 ? "-0" : "-") + std::to_string(month);
    }

    int CurrentMonthIndex(const std::time_t t_now)
    {
        std::tm utc{};
        gmtime_r(&t_now, &utc);

        return MonthIndex(utc.tm_year + 1900, utc.tm_mon + 1);
    }

    std::string Lower(std::string t_text)
    {
        std::ranges::transform(t_text, t_text.begin(),
                               [](const unsigned char t_c) { return static_cast<char>(std::tolower(t_c)); });

        return t_text;
    }

    //-------------------------------------------------
 // Requests
    //-------------------------------------------------

    void DefaultSleep(const int t_milliseconds, const std::atomic<bool>* t_cancel)
    {
        for (int waited{ 0 }; waited < t_milliseconds; waited += SLEEP_STEP_MS)
        {
            if (t_cancel != nullptr && t_cancel->load())
            {
                return;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{ SLEEP_STEP_MS });
        }
    }

 // How many seconds the server wants us to wait: the Retry-After header, kept within sensible limits.
    int RetrySeconds(const net::HttpResponse& t_response)
    {
        const std::string value{ t_response.Header("retry-after") };

        if (!value.empty() && std::ranges::all_of(value, [](const char t_c) { return t_c >= '0' && t_c <= '9'; })
            && value.size() < 6)
        {
            return std::clamp(std::stoi(value), 1, MAX_RETRY_SECONDS);
        }

        return DEFAULT_RETRY_SECONDS;
    }

 // What an answer that is not good means.
    net::FetchError ErrorOf(const net::HttpResponse& t_response)
    {
        if (t_response.cancelled)
        {
            return net::FetchError::CANCELLED;
        }

        if (!t_response.Answered())
        {
            return net::FetchError::NETWORK;
        }

        if (t_response.status == HTTP_NOT_FOUND || t_response.status == HTTP_GONE)
        {
            return net::FetchError::USER_NOT_FOUND;
        }

        return t_response.status == HTTP_TOO_MANY_REQUESTS ? net::FetchError::RATE_LIMITED : net::FetchError::SERVER;
    }

 // The words for the log: what was asked, and what came back.
    std::string Describe(const std::string& t_what, const net::HttpResponse& t_response)
    {
        if (!t_response.Answered())
        {
            return t_what + ": " + (t_response.error.empty() ? "no answer" : t_response.error);
        }

        return t_what + ": HTTP " + std::to_string(t_response.status);
    }

 // Everything a request needs, and the retries after "429".
    struct Session
    {
        net::HttpClient& http;
        const net::ClientOptions& options;
        const std::atomic<bool>* cancel;

 // Asks for a URL. If chess.com says "too many requests", it waits and asks again a few times.
        net::HttpResponse Ask(const std::string& t_url, const CacheMeta* t_cached) const
        {
            std::vector<net::HttpClient::Header> headers{ { "User-Agent", options.userAgent },
                                                          { "Accept", "application/json" } };

            if (t_cached != nullptr && !t_cached->etag.empty())
            {
                headers.emplace_back("If-None-Match", t_cached->etag);
            }
            else if (t_cached != nullptr && !t_cached->lastModified.empty())
            {
                headers.emplace_back("If-Modified-Since", t_cached->lastModified);
            }

            net::HttpResponse response;

            for (int attempt{ 0 }; attempt <= options.maxRetries; ++attempt)
            {
                response = http.Get(t_url, headers, cancel);

                if (response.status != HTTP_TOO_MANY_REQUESTS || attempt == options.maxRetries)
                {
                    break;
                }

                options.sleep(RetrySeconds(response) * 1000, cancel);

                if (cancel != nullptr && cancel->load())
                {
                    response.cancelled = true;
                    break;
                }
            }

            return response;
        }
    };

    CacheMeta MetaOf(const net::HttpResponse& t_response, const bool t_final)
    {
        return { t_response.Header("etag"), t_response.Header("last-modified"), t_final };
    }
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::net::ChessComClient::ChessComClient(HttpClient& t_http, std::filesystem::path t_cacheRoot,
                                               ClientOptions t_options)
    : m_http(t_http)
    , m_cacheRoot(std::move(t_cacheRoot))
    , m_options(std::move(t_options))
{
    if (!m_options.sleep)
    {
        m_options.sleep = DefaultSleep;
    }

    if (!m_options.now)
    {
        m_options.now = []() { return std::time(nullptr); };
    }
}

//-------------------------------------------------
// Fetching
//-------------------------------------------------

sg::chess::net::FetchResult sg::chess::net::ChessComClient::Fetch(const std::string& t_user, const int t_maxMonths,
                                                                  const ProgressFunction& t_progress,
                                                                  const std::atomic<bool>* t_cancel,
                                                                  const bool t_refreshAll)
{
    FetchResult result;

    if (!core::is_valid_chesscom_username(t_user))
    {
        result.error = FetchError::INVALID_USERNAME;
        result.message = "Not a user name of chess.com";

        return result;
    }

    const std::string user{ Lower(t_user) };
    const fs::path directory{ m_cacheRoot / user };
    const std::string prefix{ std::string{ API_ROOT } + user + "/games/" };
    const Session session{ m_http, m_options, t_cancel };

 // A first error is kept. The following ones are only consequences of it.
    const auto fail = [&](const FetchError t_error, const std::string& t_message)
    {
        if (result.error == FetchError::NONE)
        {
            result.error = t_error;
            result.message = t_message;
        }
    };

    //-------------------------------------------------
 // The list of months
    //-------------------------------------------------

    bool offline{ false }; // After a failure, the rest comes from the cache without asking again
    std::vector<std::string> archives;

    {
        const std::string url{ prefix + "archives" };
        auto cached{ LoadCache(directory, "archives") };
        auto cachedList{ cached ? core::parse_chesscom_archives(cached->body) : std::nullopt };

        if (!cachedList)
        {
            cached.reset();
        }

        const HttpResponse response{ session.Ask(url, cached ? &cached->meta : nullptr) };
        std::optional<std::vector<std::string>> list;

        if (response.status == HTTP_OK)
        {
            list = core::parse_chesscom_archives(response.body);

            if (list)
            {
                SaveCache(directory, "archives", response.body, MetaOf(response, false));
            }
            else
            {
                fail(FetchError::BAD_RESPONSE, "archives: the answer is not a list of archives");
            }
        }
        else if (response.status == HTTP_NOT_MODIFIED && cachedList)
        {
            list = cachedList;
        }
        else
        {
            const FetchError error{ ErrorOf(response) };
            fail(error, Describe("archives", response));

 // Nobody of this name, or the user has stopped: the cache has nothing to say about it.
            if (error == FetchError::USER_NOT_FOUND || error == FetchError::CANCELLED)
            {
                return result;
            }
        }

        if (!list && cachedList)
        {
            list = cachedList;
        }

        if (!list)
        {
            return result;
        }

        offline = result.error != FetchError::NONE;
        archives = std::move(*list);
    }

    //-------------------------------------------------
 // The months, the newest first
    //-------------------------------------------------

    std::vector<int> months;

    for (const std::string& archive : archives)
    {
 // Only addresses of this user's games are followed: the list comes from a server, so it is not trusted.
        if (archive.starts_with(prefix))
        {
            if (const auto index{ MonthIndexOf(archive.substr(prefix.size())) })
            {
                months.push_back(*index);
            }
        }
    }

    std::ranges::sort(months, std::greater<>{});
    months.erase(std::ranges::unique(months).begin(), months.end());

    if (t_maxMonths > 0 && months.size() > static_cast<std::size_t>(t_maxMonths))
    {
        months.resize(static_cast<std::size_t>(t_maxMonths));
        result.moreMonthsAvailable = true;
    }

    const int finalUpTo{ CurrentMonthIndex(m_options.now()) - FINAL_AFTER_MONTHS };
    int done{ 0 };

    for (const int index : months)
    {
        const std::string name{ MonthLabel(index) };

        if (t_progress)
        {
            t_progress({ done, static_cast<int>(months.size()), name });
        }

        // After the progress: the caller may have asked to stop in it.
        if (t_cancel != nullptr && t_cancel->load())
        {
            fail(FetchError::CANCELLED, "cancelled");
            break;
        }

        std::optional<core::ParsedMonth> parsed;
        auto cached{ LoadCache(directory, name) };
        std::optional<core::ParsedMonth> cachedMonth{ cached ? core::parse_chesscom_month(cached->body, user)
                                                              : std::nullopt };

        if (!cachedMonth)
        {
            cached.reset();
        }

        if (cachedMonth && (offline || (cached->meta.final && !t_refreshAll)))
        {
            parsed = std::move(cachedMonth);
            ++result.monthsFromCache;
        }
        else if (!offline)
        {
            const std::string url{ prefix + name.substr(0, 4) + "/" + name.substr(5) };
            const HttpResponse response{ session.Ask(url, cached ? &cached->meta : nullptr) };

            if (response.status == HTTP_OK)
            {
                parsed = core::parse_chesscom_month(response.body, user);

                if (parsed)
                {
                    SaveCache(directory, name, response.body, MetaOf(response, index <= finalUpTo));
                    ++result.monthsDownloaded;
                }
                else
                {
                    fail(FetchError::BAD_RESPONSE, Describe(name, response) + ", but not the games of a month");
                }
            }
            else if (response.status == HTTP_NOT_MODIFIED && cachedMonth)
            {
 // Nothing has changed since the last time. If the month is over for good now, it is final.
                CacheMeta meta{ cached->meta };
                meta.final = index <= finalUpTo;
                SaveMeta(directory, name, meta);

                parsed = std::move(cachedMonth);
                ++result.monthsFromCache;
            }
            else if (response.status == HTTP_NOT_FOUND || response.status == HTTP_GONE)
            {
 // A month in the list that is not there is no reason to stop: the other months are fine.
            }
            else
            {
                fail(ErrorOf(response), Describe(name, response));

                if (ErrorOf(response) == FetchError::CANCELLED)
                {
                    break;
                }

                offline = true;
            }

            if (!parsed && cachedMonth)
            {
                parsed = std::move(cachedMonth);
                ++result.monthsFromCache;
            }
        }

        if (parsed)
        {
            result.skippedVariants += parsed->skippedVariants;
            result.skippedBroken += parsed->skippedBroken;
            core::add_games(result.games, std::move(parsed->games));
        }

        ++done;
    }

    if (t_progress && result.error != FetchError::CANCELLED)
    {
        t_progress({ done, static_cast<int>(months.size()), {} });
    }

    return result;
}
