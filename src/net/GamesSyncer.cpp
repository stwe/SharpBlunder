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

#include <utility>
#include "net/GamesSyncer.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::net::GamesSyncer::GamesSyncer(ChessComClient& t_client)
    : m_client(t_client)
{
}

sg::chess::net::GamesSyncer::~GamesSyncer() noexcept
{
    m_cancel = true;

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

//-------------------------------------------------
// Control
//-------------------------------------------------

bool sg::chess::net::GamesSyncer::Start(const std::string& t_user, const int t_maxMonths, const bool t_refreshAll)
{
    if (m_running)
    {
        return false;
    }

    // The last worker has finished its work (m_running is false), so this does not wait.
    if (m_thread.joinable())
    {
        m_thread.join();
    }

    {
        const std::lock_guard lock{ m_mutex };
        m_progress = {};
        m_result.reset();
    }

    m_cancel = false;
    m_running = true;

    m_thread = std::thread{ [this, t_user, t_maxMonths, t_refreshAll]()
                            {
                                FetchResult result{ m_client.Fetch(
                                    t_user, t_maxMonths,
                                    [this](const FetchProgress& t_progress)
                                    {
                                        const std::lock_guard lock{ m_mutex };
                                        m_progress = t_progress;
                                    },
                                    &m_cancel, t_refreshAll) };

                                {
                                    const std::lock_guard lock{ m_mutex };
                                    m_result = std::move(result);
                                }

                                // After the result is there: whoever sees "not running" can take it.
                                m_running = false;
                            } };

    return true;
}

void sg::chess::net::GamesSyncer::Cancel()
{
    m_cancel = true;
}

//-------------------------------------------------
// State
//-------------------------------------------------

bool sg::chess::net::GamesSyncer::IsRunning() const noexcept
{
    return m_running;
}

sg::chess::net::FetchProgress sg::chess::net::GamesSyncer::Progress() const
{
    const std::lock_guard lock{ m_mutex };

    return m_progress;
}

std::optional<sg::chess::net::FetchResult> sg::chess::net::GamesSyncer::TakeResult()
{
    const std::lock_guard lock{ m_mutex };
    std::optional<FetchResult> result;
    result.swap(m_result);

    return result;
}
