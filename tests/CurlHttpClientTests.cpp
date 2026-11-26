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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include "Check.h"
#include "net/CurlHttpClient.h"

namespace
{
    namespace net = sg::chess::net;

    // How long the route /slow waits before it answers.
    constexpr int SLOW_MILLISECONDS{ 1500 };

    // A very small web server on this computer, only for these tests: it answers one request after the other and
    // closes the connection. The real service (chess.com) cannot be asked in a test.
    class LocalServer
    {
    public:
        LocalServer()
        {
            m_socket = socket(AF_INET, SOCK_STREAM, 0);

            const int yes{ 1 };
            setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = 0;  // The system chooses a free port.

            if (bind(m_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
                || listen(m_socket, 8) != 0)
            {
                std::cerr << "The test server could not be started\n";
                std::exit(EXIT_FAILURE);
            }

            socklen_t length{ sizeof(address) };
            getsockname(m_socket, reinterpret_cast<sockaddr*>(&address), &length);
            m_port = ntohs(address.sin_port);

            m_thread = std::thread{ [this]() { Serve(); } };
        }

        ~LocalServer()
        {
            m_stop = true;

            // The thread waits in accept(): one connection wakes it up.
            const int wake{ socket(AF_INET, SOCK_STREAM, 0) };
            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = htons(m_port);
            connect(wake, reinterpret_cast<sockaddr*>(&address), sizeof(address));
            close(wake);

            m_thread.join();
            close(m_socket);
        }

        LocalServer(const LocalServer&) = delete;
        LocalServer& operator=(const LocalServer&) = delete;

        std::string Url(const std::string& t_path) const
        {
            return "http://127.0.0.1:" + std::to_string(m_port) + t_path;
        }

        int Requests() const
        {
            return m_requests;
        }

    private:
        // The value of a header in the text of a request (names in lower case), or empty.
        static std::string HeaderOf(const std::string& t_request, const std::string& t_name)
        {
            std::string lower{ t_request };

            for (char& c : lower)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            const std::size_t at{ lower.find("\r\n" + t_name + ":") };

            if (at == std::string::npos)
            {
                return {};
            }

            std::size_t from{ at + t_name.size() + 3 };
            from += t_request[from] == ' ' ? 1 : 0;

            return t_request.substr(from, t_request.find("\r\n", from) - from);
        }

        static void Send(const int t_client, const std::string& t_text)
        {
            send(t_client, t_text.data(), t_text.size(), MSG_NOSIGNAL);
        }

        static std::string Answer(const std::string& t_status, const std::string& t_extra, const std::string& t_body)
        {
            return "HTTP/1.1 " + t_status + "\r\n" + t_extra + "Content-Length: " + std::to_string(t_body.size())
                   + "\r\nConnection: close\r\n\r\n" + t_body;
        }

        void Handle(const int t_client)
        {
            std::string request;
            char buffer[1024];

            while (request.find("\r\n\r\n") == std::string::npos)
            {
                const ssize_t got{ recv(t_client, buffer, sizeof(buffer), 0) };

                if (got <= 0)
                {
                    return;
                }

                request.append(buffer, static_cast<std::size_t>(got));
            }

            ++m_requests;

            const std::size_t start{ request.find(' ') + 1 };
            const std::string path{ request.substr(start, request.find(' ', start) - start) };

            if (path == "/ok")
            {
                Send(t_client, Answer("200 OK", "ETag: \"abc\"\r\nX-Test:   spaced value  \r\n", "hello"));
            }
            else if (path == "/conditional")
            {
                const bool same{ HeaderOf(request, "if-none-match") == "\"abc\"" };

                Send(t_client,
                     same ? Answer("304 Not Modified", "ETag: \"abc\"\r\n", "") : Answer("200 OK", "", "new"));
            }
            else if (path == "/agent")
            {
                Send(t_client, Answer("200 OK", "", HeaderOf(request, "user-agent")));
            }
            else if (path == "/redirect")
            {
                Send(t_client, Answer("302 Found", "Location: /ok\r\n", ""));
            }
            else if (path == "/redirect-file")
            {
                Send(t_client, Answer("302 Found", "Location: file:///etc/hostname\r\n", ""));
            }
            else if (path == "/loop")
            {
                Send(t_client, Answer("302 Found", "Location: /loop\r\n", ""));
            }
            else if (path == "/busy")
            {
                Send(t_client, Answer("429 Too Many Requests", "Retry-After: 3\r\n", "slow down"));
            }
            else if (path == "/slow")
            {
                std::this_thread::sleep_for(std::chrono::milliseconds{ SLOW_MILLISECONDS });
                Send(t_client, Answer("200 OK", "", "late"));
            }
            else
            {
                Send(t_client, Answer("404 Not Found", "", "no"));
            }
        }

        void Serve()
        {
            while (!m_stop)
            {
                const int client{ accept(m_socket, nullptr, nullptr) };

                if (client < 0)
                {
                    continue;
                }

                if (!m_stop)
                {
                    Handle(client);
                }

                close(client);
            }
        }

        int m_socket{ -1 };
        unsigned short m_port{ 0 };
        std::atomic<bool> m_stop{ false };
        std::atomic<int> m_requests{ 0 };
        std::thread m_thread;
    };

    void TestAnswers()
    {
        LocalServer server;
        net::CurlHttpClient client{ true };

        // Status, body and headers. Names of headers are in lower case, values have no spaces around them.
        const auto ok{ client.Get(server.Url("/ok"), {}, nullptr) };
        CHECK(ok.status == 200);
        CHECK(ok.Answered());
        CHECK(ok.body == "hello");
        CHECK(ok.Header("etag") == "\"abc\"");
        CHECK(ok.Header("x-test") == "spaced value");
        CHECK(ok.Header("nothing").empty());
        CHECK(ok.error.empty());

        // Status codes that are not 200 are answers, not errors.
        const auto missing{ client.Get(server.Url("/nothing-here"), {}, nullptr) };
        CHECK(missing.status == 404);
        CHECK(missing.error.empty());
        const auto busy{ client.Get(server.Url("/busy"), {}, nullptr) };
        CHECK(busy.status == 429);
        CHECK(busy.Header("retry-after") == "3");
    }

    void TestRequestHeaders()
    {
        LocalServer server;
        net::CurlHttpClient client{ true };

        const auto agent{ client.Get(server.Url("/agent"), { { "User-Agent", "SharpBlunder-test" } }, nullptr) };
        CHECK(agent.body == "SharpBlunder-test");

        // "If-None-Match" with the ETag gives 304 and no body.
        const auto same{ client.Get(server.Url("/conditional"), { { "If-None-Match", "\"abc\"" } }, nullptr) };
        CHECK(same.status == 304);
        CHECK(same.body.empty());
        const auto changed{ client.Get(server.Url("/conditional"), { { "If-None-Match", "\"zzz\"" } }, nullptr) };
        CHECK(changed.status == 200);
        CHECK(changed.body == "new");

        // A header with a line break is not sent: it could add other headers.
        const auto bad{ client.Get(server.Url("/agent"), { { "User-Agent", "a\r\nX-Evil: 1" } }, nullptr) };
        CHECK(bad.status == 200);
        CHECK(bad.body.empty());
    }

    void TestRedirects()
    {
        LocalServer server;
        net::CurlHttpClient client{ true };

        // A redirect is followed, and only the headers of the last answer are kept.
        const auto redirected{ client.Get(server.Url("/redirect"), {}, nullptr) };
        CHECK(redirected.status == 200);
        CHECK(redirected.body == "hello");
        CHECK(redirected.Header("location").empty());

        // A redirect to another kind of address (a file) is not followed.
        const auto file{ client.Get(server.Url("/redirect-file"), {}, nullptr) };
        CHECK(file.status == 0);
        CHECK(!file.error.empty());

        // An endless chain of redirects ends with an error.
        const auto loop{ client.Get(server.Url("/loop"), {}, nullptr) };
        CHECK(loop.status == 0);
        CHECK(!loop.error.empty());
    }

    void TestProtocols()
    {
        LocalServer server;

        // Without permission for plain http, nothing is sent at all.
        net::CurlHttpClient strict;
        const auto refused{ strict.Get(server.Url("/ok"), {}, nullptr) };
        CHECK(refused.status == 0);
        CHECK(!refused.error.empty());
        CHECK(server.Requests() == 0);

        CHECK(strict.Get("ftp://127.0.0.1/x", {}, nullptr).status == 0);
        CHECK(strict.Get("file:///etc/hostname", {}, nullptr).status == 0);
        CHECK(strict.Get("", {}, nullptr).status == 0);
    }

    void TestNoServer()
    {
        net::CurlHttpClient client{ true };

        // Nobody listens on port 1: no answer, with a text that tells why.
        const auto response{ client.Get("http://127.0.0.1:1/", {}, nullptr) };
        CHECK(response.status == 0);
        CHECK(!response.Answered());
        CHECK(!response.cancelled);
        CHECK(!response.error.empty());
    }

    void TestCancel()
    {
        LocalServer server;
        net::CurlHttpClient client{ true };

        // The flag is set while the server is still thinking: the request stops long before the answer.
        std::atomic<bool> cancel{ false };
        std::thread stopper{ [&]()
                             {
                                 std::this_thread::sleep_for(std::chrono::milliseconds{ 300 });
                                 cancel = true;
                             } };

        const auto start{ std::chrono::steady_clock::now() };
        const auto response{ client.Get(server.Url("/slow"), {}, &cancel) };
        const auto elapsed{ std::chrono::steady_clock::now() - start };
        stopper.join();

        CHECK(response.cancelled);
        CHECK(response.status == 0);
        CHECK(elapsed < std::chrono::milliseconds{ SLOW_MILLISECONDS - 300 });

        // A flag that is set from the start: no request is needed to know the answer.
        std::atomic<bool> already{ true };
        CHECK(client.Get(server.Url("/ok"), {}, &already).cancelled);
    }
} // namespace

int main()
{
    TestAnswers();
    TestRequestHeaders();
    TestRedirects();
    TestProtocols();
    TestNoServer();
    TestCancel();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
