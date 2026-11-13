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

#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>
#include "engine/Engine.h"

namespace
{
    void CloseIfOpen(int& t_fd) noexcept
    {
        if (t_fd >= 0)
        {
            close(t_fd);
            t_fd = -1;
        }
    }
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::engine::Engine::Engine(const std::string& t_executable)
{
    // O_CLOEXEC: the pipe ends are closed automatically in the engine process after exec.
    int toEngine[2]{ -1, -1 };
    int fromEngine[2]{ -1, -1 };

    if (pipe2(toEngine, O_CLOEXEC) != 0 || pipe2(fromEngine, O_CLOEXEC) != 0)
    {
        CloseIfOpen(toEngine[0]);
        CloseIfOpen(toEngine[1]);
        CloseIfOpen(fromEngine[0]);
        CloseIfOpen(fromEngine[1]);

        throw std::runtime_error("Cannot create pipes for the engine");
    }

    m_pid = fork();
    if (m_pid < 0)
    {
        CloseIfOpen(toEngine[0]);
        CloseIfOpen(toEngine[1]);
        CloseIfOpen(fromEngine[0]);
        CloseIfOpen(fromEngine[1]);

        throw std::runtime_error("Cannot start a process for the engine");
    }

    if (m_pid == 0)
    {
        // Child: connect the pipes to stdin and stdout, then become the engine.
        dup2(toEngine[0], STDIN_FILENO);
        dup2(fromEngine[1], STDOUT_FILENO);

        execlp(t_executable.c_str(), t_executable.c_str(), static_cast<char*>(nullptr));
        execl("/usr/games/stockfish", "stockfish", static_cast<char*>(nullptr)); // Debian/Ubuntu location

        _exit(127); // neither worked
    }

    // Parent
    close(toEngine[0]);
    close(fromEngine[1]);

    m_writeFd = toEngine[1];
    m_reader = fdopen(fromEngine[0], "r");
    if (m_reader == nullptr)
    {
        close(fromEngine[0]);
        Shutdown();

        throw std::runtime_error("Cannot read from the engine");
    }

    // If the engine could not be started, the pipe is closed right away and the handshake fails.
    try
    {
        Send("uci");
        static_cast<void>(ReadUntil("uciok"));
    }
    catch (const std::exception&)
    {
        Shutdown();

        throw std::runtime_error("Cannot start Stockfish ('" + t_executable +
                                 "'). Is it installed? (Arch/CachyOS: sudo pacman -S stockfish)");
    }
}

sg::chess::engine::Engine::~Engine() noexcept
{
    Shutdown();
}

//-------------------------------------------------
// Communication
//-------------------------------------------------

void sg::chess::engine::Engine::Send(const std::string_view t_command) const
{
    std::string line{ t_command };
    line += '\n';

    const char* data{ line.data() };
    std::size_t remaining{ line.size() };

    while (remaining > 0)
    {
        const ssize_t written{ write(m_writeFd, data, remaining) };
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw std::runtime_error("Cannot write to the engine");
        }

        data += written;
        remaining -= static_cast<std::size_t>(written);
    }
}

std::vector<std::string> sg::chess::engine::Engine::ReadUntil(const std::string_view t_prefix)
{
    std::vector<std::string> lines;
    std::string line;

    while (ReadLine(line))
    {
        const bool done{ line.starts_with(t_prefix) };
        lines.push_back(line);

        if (done)
        {
            return lines;
        }
    }

    throw std::runtime_error("The engine ended unexpectedly");
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

bool sg::chess::engine::Engine::ReadLine(std::string& t_line) const
{
    t_line.clear();

    char chunk[512];
    while (fgets(chunk, sizeof chunk, m_reader) != nullptr)
    {
        t_line += chunk;

        if (t_line.back() == '\n')
        {
            while (!t_line.empty() && (t_line.back() == '\n' || t_line.back() == '\r'))
            {
                t_line.pop_back();
            }

            return true;
        }
    }

    return false;
}

void sg::chess::engine::Engine::Shutdown() noexcept
{
    if (m_pid > 0)
    {
        [[maybe_unused]] const ssize_t written{ write(m_writeFd, "quit\n", 5) };

        CloseIfOpen(m_writeFd);
        kill(m_pid, SIGTERM);
        waitpid(m_pid, nullptr, 0);
        m_pid = -1;
    }

    CloseIfOpen(m_writeFd);

    if (m_reader != nullptr)
    {
        fclose(m_reader);
        m_reader = nullptr;
    }
}
