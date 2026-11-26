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

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "Check.h"
#include "FakeHttpClient.h"
#include "net/ChessComClient.h"

namespace
{
    namespace fs = std::filesystem;
    namespace net = sg::chess::net;
    using sg::chess::tests::FakeHttpClient;

    const std::string BASE{ "https://api.chess.com/pub/player/steffengo/games/" };
    const std::string ARCHIVES_URL{ BASE + "archives" };
    const std::string NOVEMBER_URL{ BASE + "2026/11" };
    const std::string OCTOBER_URL{ BASE + "2026/10" };

    // 2026-11-26 and 2027-02-15 (UTC), the "current time" of the tests.
    constexpr std::time_t NOVEMBER_26{ 1795651200 };
    constexpr std::time_t FEBRUARY_15{ 1802649600 };

    std::string ReadFileText(const fs::path& t_path)
    {
        std::ifstream file{ t_path, std::ios::binary };

        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }

    // The recorded answer of chess.com for November (opponents are made anonymous), see tests/data.
    std::string November()
    {
        return ReadFileText(fs::path{ SHARPBLUNDER_TEST_DATA_DIR } / "chesscom_2026-11.json");
    }

    std::string Archives()
    {
        return "{\"archives\":[\"" + OCTOBER_URL + "\",\"" + NOVEMBER_URL + "\"]}";
    }

    // A new empty folder for the cache of a test.
    fs::path MakeFolder(const std::string& t_name)
    {
        const fs::path folder{ fs::temp_directory_path() / ("sharpblunder-client-" + t_name) };

        fs::remove_all(folder);
        fs::create_directories(folder);

        return folder;
    }

    // The client of a test: no waiting (the waits are recorded), and a clock that the test sets.
    struct Setup
    {
        FakeHttpClient http;
        std::vector<int> waits;
        std::time_t now{ NOVEMBER_26 };
        fs::path folder;
        net::ClientOptions options;

        explicit Setup(const std::string& t_name)
            : folder(MakeFolder(t_name))
        {
            options.userAgent = "SharpBlunder-test (+https://example.org)";
            options.sleep = [this](const int t_ms, const std::atomic<bool>*) { waits.push_back(t_ms); };
            options.now = [this]() { return now; };
        }

        ~Setup()
        {
            fs::remove_all(folder);
        }

        // Fetches with a new client, as the program does after a restart. The cache is on disk.
        net::FetchResult Fetch(const std::string& t_user = "SteffenGo", const int t_months = 0,
                               const std::atomic<bool>* t_cancel = nullptr, const bool t_refreshAll = false)
        {
            net::ChessComClient client{ http, folder, options };

            return client.Fetch(t_user, t_months, {}, t_cancel, t_refreshAll);
        }

        void AddNormal()
        {
            http.AddOk(ARCHIVES_URL, Archives(), "\"a1\"");
            http.AddOk(NOVEMBER_URL, November(), "\"n1\"");
            http.AddOk(OCTOBER_URL, "{\"games\":[]}", "\"o1\"");
        }
    };

    void TestNormal()
    {
        Setup setup{ "normal" };
        setup.AddNormal();

        const auto result{ setup.Fetch() };

        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.size() == 8);
        CHECK(result.monthsDownloaded == 2);
        CHECK(result.monthsFromCache == 0);
        CHECK(!result.moreMonthsAvailable);

        // The newest game first. The user is White in it.
        CHECK(result.games[0].white == "SteffenGo");
        CHECK(result.games[0].endTime > result.games[7].endTime);
        CHECK(result.games[0].userColor == sg::chess::core::Color::WHITE);

        // Archives first, then the newest month. A User-Agent with contact data is sent every time.
        const auto& requests{ setup.http.Requests() };
        CHECK(requests.size() == 3);
        CHECK(requests[0].url == ARCHIVES_URL);
        CHECK(requests[1].url == NOVEMBER_URL);
        CHECK(requests[2].url == OCTOBER_URL);

        for (const auto& request : requests)
        {
            CHECK(request.Value("User-Agent") == "SharpBlunder-test (+https://example.org)");
            CHECK(request.Value("If-None-Match").empty());
        }

        // The cache folder is named after the user in lower case.
        CHECK(fs::exists(setup.folder / "steffengo" / "archives.json"));
        CHECK(fs::exists(setup.folder / "steffengo" / "2026-11.json"));
        CHECK(fs::exists(setup.folder / "steffengo" / "2026-11.meta.json"));
        CHECK(ReadFileText(setup.folder / "steffengo" / "2026-11.json") == November());
        CHECK(!fs::exists(setup.folder / "steffengo" / "2026-11.json.tmp"));
    }

    void TestCacheAndEtag()
    {
        Setup setup{ "etag" };
        setup.AddNormal();
        CHECK(setup.Fetch().games.size() == 8);

        // The server says "not modified" to everything that is asked with an ETag.
        setup.http.AddStatus(ARCHIVES_URL, 304);
        setup.http.AddStatus(NOVEMBER_URL, 304);
        setup.http.AddStatus(OCTOBER_URL, 304);

        // Both months are young, so they are asked again, with the ETag of the first answer.
        const auto second{ setup.Fetch() };
        CHECK(second.error == net::FetchError::NONE);
        CHECK(second.games.size() == 8);
        CHECK(second.monthsDownloaded == 0);
        CHECK(second.monthsFromCache == 2);
        CHECK(setup.http.Count(NOVEMBER_URL) == 2);
        CHECK(setup.http.Requests()[3].Value("If-None-Match") == "\"a1\"");
        CHECK(setup.http.Requests()[4].Value("If-None-Match") == "\"n1\"");

        // Three months later, "not modified" makes both months final: they are never asked for again.
        setup.now = FEBRUARY_15;
        CHECK(setup.Fetch().games.size() == 8);
        CHECK(setup.http.Count(NOVEMBER_URL) == 3);
        const int before{ setup.http.Count(NOVEMBER_URL) };

        const auto fourth{ setup.Fetch() };
        CHECK(fourth.games.size() == 8);
        CHECK(fourth.monthsFromCache == 2);
        CHECK(setup.http.Count(NOVEMBER_URL) == before);
        CHECK(setup.http.Count(OCTOBER_URL) == before);
        CHECK(setup.http.Count(ARCHIVES_URL) == 4);

        // "Refresh all" asks for them again, still with the ETag.
        const auto refreshed{ setup.Fetch("SteffenGo", 0, nullptr, true) };
        CHECK(refreshed.games.size() == 8);
        CHECK(setup.http.Count(NOVEMBER_URL) == before + 1);
        CHECK(setup.http.Requests().back().Value("If-None-Match") == "\"o1\"");
    }

    void TestChangedMonth()
    {
        Setup setup{ "changed" };
        setup.AddNormal();
        CHECK(setup.Fetch().games.size() == 8);

        // A month that has changed comes again with a body, and replaces the old one.
        setup.http.AddStatus(ARCHIVES_URL, 304);
        setup.http.AddOk(NOVEMBER_URL, "{\"games\":[]}", "\"n2\"");
        setup.http.AddStatus(OCTOBER_URL, 304);

        const auto result{ setup.Fetch() };
        CHECK(result.games.empty());
        CHECK(result.monthsDownloaded == 1);
        CHECK(ReadFileText(setup.folder / "steffengo" / "2026-11.json") == "{\"games\":[]}");
    }

    void TestMonthLimit()
    {
        Setup setup{ "limit" };
        setup.AddNormal();

        // Only the newest month: the older one is not asked for. The caller learns that there is more.
        const auto result{ setup.Fetch("SteffenGo", 1) };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.size() == 8);
        CHECK(result.moreMonthsAvailable);
        CHECK(setup.http.Count(OCTOBER_URL) == 0);

        const auto all{ setup.Fetch("SteffenGo", 2) };
        CHECK(!all.moreMonthsAvailable);
        CHECK(setup.http.Count(OCTOBER_URL) == 1);
    }

    void TestUnknownUser()
    {
        Setup setup{ "unknown" };
        setup.http.AddStatus(ARCHIVES_URL, 404);

        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::USER_NOT_FOUND);
        CHECK(result.games.empty());
        CHECK(setup.http.Requests().size() == 1);
        CHECK(!result.message.empty());
    }

    void TestInvalidUsername()
    {
        Setup setup{ "invalid" };

        // Nothing that could change the address is sent to the server.
        for (const char* name : { "", "a/b", "../x", "a b", "a?b" })
        {
            CHECK(setup.Fetch(name).error == net::FetchError::INVALID_USERNAME);
        }

        CHECK(setup.http.Requests().empty());
    }

    void TestRateLimit()
    {
        Setup setup{ "rate" };

        // Two times "too many requests", then it works. The wait of the server is honored.
        net::HttpResponse busy;
        busy.status = 429;
        busy.headers["retry-after"] = "7";
        setup.http.Add(ARCHIVES_URL, busy);
        setup.http.Add(ARCHIVES_URL, busy);
        setup.http.AddOk(ARCHIVES_URL, Archives(), "\"a1\"");
        setup.http.AddOk(NOVEMBER_URL, November());
        setup.http.AddOk(OCTOBER_URL, "{\"games\":[]}");

        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.size() == 8);
        CHECK(setup.waits == (std::vector<int>{ 7000, 7000 }));
        CHECK(setup.http.Count(ARCHIVES_URL) == 3);
    }

    void TestRateLimitStays()
    {
        Setup setup{ "ratestays" };
        setup.http.AddStatus(ARCHIVES_URL, 429);

        // Without Retry-After: the default wait. After the third try, it gives up.
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::RATE_LIMITED);
        CHECK(result.games.empty());
        CHECK(setup.http.Count(ARCHIVES_URL) == 3);
        CHECK(setup.waits == (std::vector<int>{ 5000, 5000 }));
    }

    void TestNetworkErrorUsesCache()
    {
        Setup setup{ "offline" };
        setup.AddNormal();
        CHECK(setup.Fetch().games.size() == 8);

        // The network is gone: the games come from the cache, and the error is reported.
        setup.http.AddNetworkError(ARCHIVES_URL, "could not resolve host");
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NETWORK);
        CHECK(result.games.size() == 8);
        CHECK(result.monthsFromCache == 2);
        CHECK(result.message.find("could not resolve host") != std::string::npos);

        // It does not try the months one by one when the first request has shown that there is no network.
        CHECK(setup.http.Count(NOVEMBER_URL) == 1);
    }

    void TestNetworkErrorWithoutCache()
    {
        Setup setup{ "nocache" };
        setup.http.AddNetworkError(ARCHIVES_URL, "timeout");

        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NETWORK);
        CHECK(result.games.empty());
    }

    void TestFailureInTheMiddle()
    {
        Setup setup{ "middle" };
        setup.http.AddOk(ARCHIVES_URL, Archives());
        setup.http.AddOk(NOVEMBER_URL, November());
        setup.http.AddStatus(OCTOBER_URL, 500);

        // The month that was fetched is kept. The error is reported.
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::SERVER);
        CHECK(result.games.size() == 8);
        CHECK(result.monthsDownloaded == 1);
    }

    void TestBrokenAnswers()
    {
        Setup setup{ "broken" };
        setup.http.AddOk(ARCHIVES_URL, Archives());
        setup.http.AddOk(NOVEMBER_URL, "{\"games\": [");
        setup.http.AddOk(OCTOBER_URL, "{\"games\":[]}");

        // A broken month is not put into the cache. The next month is still fetched.
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::BAD_RESPONSE);
        CHECK(result.games.empty());
        CHECK(!fs::exists(setup.folder / "steffengo" / "2026-11.json"));
        CHECK(fs::exists(setup.folder / "steffengo" / "2026-10.json"));

        // A broken list of archives.
        Setup other{ "brokenlist" };
        other.http.AddOk(ARCHIVES_URL, "<html>maintenance</html>");
        CHECK(other.Fetch().error == net::FetchError::BAD_RESPONSE);
        CHECK(!fs::exists(other.folder / "steffengo" / "archives.json"));
    }

    void TestBrokenCache()
    {
        Setup setup{ "brokencache" };
        setup.AddNormal();
        CHECK(setup.Fetch().games.size() == 8);

        // A damaged cache file is not trusted: the month is downloaded again, without an ETag.
        std::ofstream{ setup.folder / "steffengo" / "2026-11.json", std::ios::trunc } << "{\"games\":[";
        setup.http.AddOk(NOVEMBER_URL, November(), "\"n9\"");
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.size() == 8);

        // The last request for November was made without an ETag.
        std::string etagSent{ "none" };

        for (const auto& request : setup.http.Requests())
        {
            etagSent = request.url == NOVEMBER_URL ? request.Value("If-None-Match") : etagSent;
        }

        CHECK(etagSent.empty());
        CHECK(ReadFileText(setup.folder / "steffengo" / "2026-11.json") == November());
    }

    void TestForeignAddresses()
    {
        Setup setup{ "foreign" };
        const std::string evil{ "https://evil.example/pub/player/steffengo/games/2026/09" };
        const std::string other{ "https://api.chess.com/pub/player/someoneelse/games/2026/09" };
        const std::string odd{ BASE + "2026/13" };
        const std::string text{ BASE + "latest" };
        setup.http.AddOk(ARCHIVES_URL, "{\"archives\":[\"" + evil + "\",\"" + other + "\",\"" + odd + "\",\"" + text
                                           + "\",\"" + NOVEMBER_URL + "\"]}");
        setup.http.AddOk(NOVEMBER_URL, November());

        // Addresses of other hosts, other users or of a form that is not a month are not asked for.
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(setup.http.Requests().size() == 2);
        CHECK(setup.http.Count(evil) == 0);
        CHECK(setup.http.Count(other) == 0);
    }

    void TestMissingMonth()
    {
        Setup setup{ "missing" };
        setup.http.AddOk(ARCHIVES_URL, Archives());
        setup.http.AddOk(NOVEMBER_URL, November());
        setup.http.AddStatus(OCTOBER_URL, 404);

        // A month that is in the list but gone is no error.
        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.size() == 8);
    }

    void TestVariants()
    {
        Setup setup{ "variants" };
        setup.http.AddOk(ARCHIVES_URL, "{\"archives\":[\"" + NOVEMBER_URL + "\"]}");
        setup.http.AddOk(NOVEMBER_URL,
                         "{\"games\":[{\"rules\":\"chess960\",\"pgn\":\"x\",\"uuid\":\"1\"},{\"x\":1}]}");

        const auto result{ setup.Fetch() };
        CHECK(result.error == net::FetchError::NONE);
        CHECK(result.games.empty());
        CHECK(result.skippedVariants == 1);
        CHECK(result.skippedBroken == 1);
    }

    void TestCancel()
    {
        Setup setup{ "cancel" };
        setup.AddNormal();

        std::atomic<bool> cancel{ true };
        const auto result{ setup.Fetch("SteffenGo", 0, &cancel) };
        CHECK(result.error == net::FetchError::CANCELLED);
        CHECK(result.games.empty());

        // Cancelled between two months: what is there is returned.
        Setup second{ "cancel2" };
        second.AddNormal();
        std::atomic<bool> stop{ false };
        net::ChessComClient client{ second.http, second.folder, second.options };
        const auto partial{ client.Fetch(
            "SteffenGo", 0, [&](const net::FetchProgress& t_progress) { stop = t_progress.done == 1; }, &stop) };
        CHECK(partial.error == net::FetchError::CANCELLED);
        CHECK(partial.games.size() == 8);
        CHECK(second.http.Count(OCTOBER_URL) == 0);
    }

    void TestProgress()
    {
        Setup setup{ "progress" };
        setup.AddNormal();

        std::vector<net::FetchProgress> steps;
        net::ChessComClient client{ setup.http, setup.folder, setup.options };
        const auto result{ client.Fetch(
            "SteffenGo", 0, [&](const net::FetchProgress& t_progress) { steps.push_back(t_progress); }, nullptr) };

        CHECK(result.error == net::FetchError::NONE);
        CHECK(steps.size() == 3);
        CHECK(steps[0].done == 0 && steps[0].total == 2 && steps[0].month == "2026-11");
        CHECK(steps[1].done == 1 && steps[1].month == "2026-10");
        CHECK(steps[2].done == 2);
    }
} // namespace

int main()
{
    TestNormal();
    TestCacheAndEtag();
    TestChangedMonth();
    TestMonthLimit();
    TestUnknownUser();
    TestInvalidUsername();
    TestRateLimit();
    TestRateLimitStays();
    TestNetworkErrorUsesCache();
    TestNetworkErrorWithoutCache();
    TestFailureInTheMiddle();
    TestBrokenAnswers();
    TestBrokenCache();
    TestForeignAddresses();
    TestMissingMonth();
    TestVariants();
    TestCancel();
    TestProgress();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
