#include <algorithm>
#include "core/GameState.h"
#include "core/San.h"

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::core::GameState::GameState(RulesProvider& t_rules)
    : GameState(t_rules, Position::FromFen(Position::START_FEN))
{
}

sg::chess::core::GameState::GameState(RulesProvider& t_rules, const Position& t_start)
    : m_rules(t_rules)
{
    m_entries.push_back({ m_rules.Describe(t_start), std::nullopt, "" });
}

//-------------------------------------------------
// Navigation
//-------------------------------------------------

void sg::chess::core::GameState::GoBack() noexcept
{
    if (CanGoBack())
    {
        --m_current;
    }
}

void sg::chess::core::GameState::GoForward() noexcept
{
    if (CanGoForward())
    {
        ++m_current;
    }
}

void sg::chess::core::GameState::GoToStart() noexcept
{
    m_current = 0;
}

void sg::chess::core::GameState::GoToEnd() noexcept
{
    m_current = PlyCount();
}

void sg::chess::core::GameState::GoTo(const std::size_t t_ply) noexcept
{
    m_current = std::min(t_ply, PlyCount());
}

//-------------------------------------------------
// Changes
//-------------------------------------------------

bool sg::chess::core::GameState::Play(const Move& t_move)
{
    const PositionInfo& before{ Current().info };

    if (!before.IsLegal(t_move))
    {
        return false;
    }

    // The same move as the next one of the game: do not throw the rest of the game away.
    if (CanGoForward() && m_entries[m_current + 1].move == t_move)
    {
        ++m_current;
        return true;
    }

    PositionInfo after{ m_rules.Play(before.position, t_move) };
    std::string san{ to_san(before, t_move, after) };

    // 'before' refers into m_entries, so it must not be used after the vector changes.
    m_entries.resize(m_current + 1);
    m_entries.push_back({ std::move(after), t_move, std::move(san) });
    ++m_current;

    return true;
}

void sg::chess::core::GameState::Reset() noexcept
{
    m_entries.resize(1);
    m_current = 0;
}
