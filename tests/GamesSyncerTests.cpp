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
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include "Check.h"
#include "FakeHttpClient.h"
#include "net/GamesSyncer.h"

namespace
{
    namespace fs = std::filesystem;
    namespace net = sg::chess::net;

    const std::string ARCHIVES_URL{ "https://api.chess.com/pub/player/me/games/archives" };
    const std::string MONTH_URL{ "https://api.chess.com/pub/player/me/games/2026/11" };

    // A web client that does not answer until the test says so. It stops when the cancel flag is set.
    class GatedClient final : public net::HttpClient
    {
    public:
        // Lets the waiting request go on.
        void Open()
        {
            {
                const std::lock_guard lock{ m_mutex };
                m_open = true;
            }

            m_condition.notify_all();
        }

        [[nodiscard]] net::HttpResponse Get(const std::string& t_url, const std::vector<Header>&,
                                            const std::atomic<bool>* t_cancel) override
        {
            ++m_requests;

            // Waits until opened or cancelled. The flag is looked at every few milliseconds.
            std::unique_lock lock{ m_mutex };

            while (!m_open)
            {
                if (t_cancel != nullptr && t_cancel->load())
                {
                    net::HttpResponse response;
                    response.cancelled = true;
                    response.error = "cancelled";

                    return response;
                }

                m_condition.wait_for(lock, std::chrono::milliseconds{ 5 });
            }

            net::HttpResponse response;
            response.status = 200;
            response.body = t_url == ARCHIVES_URL ? "{\"archives\":[\"" + MONTH_URL + "\"]}" : "{\"games\":[]}";

            return response;
        }

        int Requests() const
        {
            return m_requests;
        }

    private:
        std::mutex m_mutex;
        std::condition_variable m_condition;
        bool m_open{ false };
        std::atomic<int> m_requests{ 0 };
    };

    fs::path MakeFolder(const std::string& t_name)
    {
        const fs::path folder{ fs::temp_directory_path() / ("sharpblunder-syncer-" + t_name) };

        fs::remove_all(folder);
        fs::create_directories(folder);

        return folder;
    }

    // Waits for a condition, but not for ever: a test that hangs is worse than one that fails.
    template <typename F>
    bool WaitFor(F t_condition)
    {
        const auto end{ std::chrono::steady_clock::now() + std::chrono::seconds{ 10 } };

        while (std::chrono::steady_clock::now() < end)
        {
            if (t_condition())
            {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
        }

        return false;
    }

    net::ClientOptions Options()
    {
        net::ClientOptions options;
        options.userAgent = "SharpBlunder-test";
        options.sleep = [](int, const std::atomic<bool>*) {};

        return options;
    }

    void TestRun()
    {
        const fs::path folder{ MakeFolder("run") };
        GatedClient http;
        net::ChessComClient client{ http, folder, Options() };
        net::GamesSyncer syncer{ client };

        // Nothing yet.
        CHECK(!syncer.IsRunning());
        CHECK(!syncer.TakeResult().has_value());

        // The first request waits: the syncer is running, and the window is free.
        CHECK(syncer.Start("Me", 0));
        CHECK(syncer.IsRunning());
        CHECK(WaitFor([&]() { return http.Requests() == 1; }));

        // A second start while it runs does nothing.
        CHECK(!syncer.Start("Me", 0));
        CHECK(!syncer.TakeResult().has_value());

        http.Open();
        CHECK(WaitFor([&]() { return !syncer.IsRunning(); }));

        // The result comes once, and the progress shows the end.
        const auto result{ syncer.TakeResult() };
        CHECK(result.has_value());
        CHECK(result->error == net::FetchError::NONE);
        CHECK(syncer.Progress().done == 1);
        CHECK(syncer.Progress().total == 1);
        CHECK(!syncer.TakeResult().has_value());

        // After the end, a new fetch can start.
        CHECK(syncer.Start("Me", 0));
        CHECK(WaitFor([&]() { return !syncer.IsRunning(); }));
        CHECK(syncer.TakeResult().has_value());

        fs::remove_all(folder);
    }

    void TestCancel()
    {
        const fs::path folder{ MakeFolder("cancel") };
        GatedClient http;
        net::ChessComClient client{ http, folder, Options() };
        net::GamesSyncer syncer{ client };

        CHECK(syncer.Start("Me", 0));
        CHECK(WaitFor([&]() { return http.Requests() == 1; }));

        // The request that waits is stopped, and the result says so.
        syncer.Cancel();
        CHECK(WaitFor([&]() { return !syncer.IsRunning(); }));
        const auto result{ syncer.TakeResult() };
        CHECK(result.has_value());
        CHECK(result->error == net::FetchError::CANCELLED);

        // Cancel without a running fetch is fine.
        syncer.Cancel();

        // A cancelled fetch does not stop the next one.
        http.Open();
        CHECK(syncer.Start("Me", 0));
        CHECK(WaitFor([&]() { return !syncer.IsRunning(); }));
        CHECK(syncer.TakeResult()->error == net::FetchError::NONE);

        fs::remove_all(folder);
    }

    void TestDestructorStopsTheWorker()
    {
        const fs::path folder{ MakeFolder("destructor") };
        GatedClient http;
        net::ChessComClient client{ http, folder, Options() };

        {
            net::GamesSyncer syncer{ client };
            CHECK(syncer.Start("Me", 0));
            CHECK(WaitFor([&]() { return http.Requests() == 1; }));

            // The syncer goes away while the worker waits: it must not hang, and must not crash.
        }

        CHECK(http.Requests() == 1);
        fs::remove_all(folder);
    }

    void TestInvalidName()
    {
        const fs::path folder{ MakeFolder("invalid") };
        sg::chess::tests::FakeHttpClient http;
        net::ChessComClient client{ http, folder, Options() };
        net::GamesSyncer syncer{ client };

        CHECK(syncer.Start("not a name!", 0));
        CHECK(WaitFor([&]() { return !syncer.IsRunning(); }));
        CHECK(syncer.TakeResult()->error == net::FetchError::INVALID_USERNAME);
        CHECK(http.Requests().empty());

        fs::remove_all(folder);
    }
} // namespace

int main()
{
    TestRun();
    TestCancel();
    TestDestructorStopsTheWorker();
    TestInvalidName();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
