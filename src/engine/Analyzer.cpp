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
#include <stdexcept>
#include <utility>
#include "engine/Analyzer.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::engine::Analyzer::Analyzer(const std::string& t_executable, const int t_lineCount)
    : m_engine(t_executable)
    , m_lineCount(std::max(1, t_lineCount))
{
    m_engine.Send("setoption name MultiPV value " + std::to_string(m_lineCount));

    m_reader = std::thread(&Analyzer::ReadLoop, this);
}

sg::chess::engine::Analyzer::~Analyzer() noexcept
{
    // The engine ends, its output ends, and so the reading thread ends. Only then the pipes are closed.
    m_engine.Terminate();

    if (m_reader.joinable())
    {
        m_reader.join();
    }
}

//-------------------------------------------------
// Control
//-------------------------------------------------

void sg::chess::engine::Analyzer::SetPosition(const core::Position& t_position)
{
    std::string fen{ t_position.ToFen() };

    {
        const std::lock_guard lock{ m_mutex };

        if (fen == m_fen)
        {
            return;
        }

        m_fen = std::move(fen);
        m_whiteToMove = t_position.SideToMove() == core::Color::WHITE;
    }

    Restart();
}

void sg::chess::engine::Analyzer::SetRunning(const bool t_running)
{
    {
        const std::lock_guard lock{ m_mutex };

        if (t_running == m_running)
        {
            return;
        }

        m_running = t_running;
    }

    Restart();
}

//-------------------------------------------------
// Query
//-------------------------------------------------

bool sg::chess::engine::Analyzer::IsRunning() const
{
    const std::lock_guard lock{ m_mutex };

    return m_running;
}

std::vector<sg::chess::engine::AnalysisLine> sg::chess::engine::Analyzer::Lines() const
{
    const std::lock_guard lock{ m_mutex };

    return m_lines;
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

void sg::chess::engine::Analyzer::Restart()
{
    std::string fen;
    bool search{ false };

    {
        const std::lock_guard lock{ m_mutex };

        m_lines.clear();

        search = m_running && !m_fen.empty();
        m_accepting = search;

        if (search)
        {
            // From now on, only the lines of this new search count.
            ++m_started;
            fen = m_fen;
        }
    }

    // The commands are sent without the lock, so a slow pipe never blocks the reading thread.
    // "stop" does nothing if no search is running.
    Send("stop");

    if (search)
    {
        Send("position fen " + fen);
        Send("go infinite");
    }
}

void sg::chess::engine::Analyzer::Send(const std::string_view t_command)
{
    try
    {
        m_engine.Send(t_command);
    }
    catch (const std::runtime_error&)
    {
        m_failed = true;
    }
}

void sg::chess::engine::Analyzer::ReadLoop()
{
    std::string line;

    while (m_engine.ReadLine(line))
    {
        if (line.starts_with("bestmove"))
        {
            // Every "go" is answered by exactly one "bestmove", also after a "stop".
            const std::lock_guard lock{ m_mutex };
            ++m_finished;
        }
        else if (auto parsed{ parse_info_line(line) })
        {
            Accept(std::move(*parsed));
        }
    }

    m_failed = true;
}

void sg::chess::engine::Analyzer::Accept(AnalysisLine t_line)
{
    const std::lock_guard lock{ m_mutex };

    // A line of an older search: its "bestmove" has not arrived yet, so m_finished is behind.
    if (!m_accepting || m_finished + 1 != m_started || t_line.multipv > m_lineCount)
    {
        return;
    }

    t_line.score = t_line.score.FromSideToMove(m_whiteToMove);

    const auto same{ std::ranges::find(m_lines, t_line.multipv, &AnalysisLine::multipv) };
    if (same != m_lines.end())
    {
        *same = std::move(t_line);
        return;
    }

    m_lines.push_back(std::move(t_line));
    std::ranges::sort(m_lines, {}, &AnalysisLine::multipv);
}
