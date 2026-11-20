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
#include <utility>
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
    m_nodes.push_back({ m_rules.Describe(t_start), std::nullopt, "", std::nullopt, {}, {}, "" });
}

//-------------------------------------------------
// Query
//-------------------------------------------------

bool sg::chess::core::GameState::IsMainLine(const NodeId t_id) const noexcept
{
    NodeId id{ t_id };

    while (m_nodes[id].parent)
    {
        const NodeId parent{ *m_nodes[id].parent };
        if (m_nodes[parent].children.front() != id)
        {
            return false;
        }

        id = parent;
    }

    return true;
}

//-------------------------------------------------
// Navigation
//-------------------------------------------------

void sg::chess::core::GameState::GoBack() noexcept
{
    if (CanGoBack())
    {
        m_current = *m_nodes[m_current].parent;
    }
}

void sg::chess::core::GameState::GoForward() noexcept
{
    if (CanGoForward())
    {
        m_current = m_nodes[m_current].children.front();
    }
}

void sg::chess::core::GameState::GoToStart() noexcept
{
    m_current = 0;
}

void sg::chess::core::GameState::GoToEnd() noexcept
{
    while (CanGoForward())
    {
        GoForward();
    }
}

void sg::chess::core::GameState::GoTo(const NodeId t_id) noexcept
{
    if (t_id < m_nodes.size())
    {
        m_current = t_id;
    }
}

//-------------------------------------------------
// Changes
//-------------------------------------------------

bool sg::chess::core::GameState::Play(const Move& t_move)
{
    if (!Current().info.IsLegal(t_move))
    {
        return false;
    }

    // The game has this move here already: do not add it a second time.
    for (const NodeId child : Current().children)
    {
        if (m_nodes[child].move == t_move)
        {
            m_current = child;
            return true;
        }
    }

    PositionInfo after{ m_rules.Play(Current().info.position, t_move) };
    std::string san{ to_san(Current().info, t_move, after) };

    // The new node is appended last: a position keeps the order of its moves, the first one is the main line.
    // The vector may grow here, so no reference into it is kept.
    const NodeId id{ m_nodes.size() };
    m_nodes.push_back({ std::move(after), t_move, std::move(san), m_current, {}, {}, "" });
    m_nodes[m_current].children.push_back(id);
    m_current = id;

    return true;
}

void sg::chess::core::GameState::SetAnnotation(const NodeId t_id, Annotation t_note)
{
    if (t_id < m_nodes.size())
    {
        m_nodes[t_id].note = std::move(t_note);
    }
}

void sg::chess::core::GameState::SetLineComment(const NodeId t_id, std::string t_comment)
{
    if (t_id < m_nodes.size())
    {
        m_nodes[t_id].lineComment = std::move(t_comment);
    }
}

void sg::chess::core::GameState::Promote(const NodeId t_id)
{
    if (t_id == 0 || t_id >= m_nodes.size())
    {
        return;
    }

    NodeId id{ t_id };

    while (m_nodes[id].parent)
    {
        const NodeId parent{ *m_nodes[id].parent };
        auto& siblings{ m_nodes[parent].children };

        // Move the node to the front. The others keep their order.
        const auto it{ std::ranges::find(siblings, id) };
        if (it != siblings.begin())
        {
            // The line is not a variation any more, so its line comment would not be shown. It stays as a part of
            // the comment on the first move.
            Node& node{ m_nodes[id] };
            if (!node.lineComment.empty())
            {
                node.note.comment = node.lineComment + (node.note.comment.empty() ? "" : "\n" + node.note.comment);
                node.lineComment.clear();
            }

            std::rotate(siblings.begin(), it, it + 1);
        }

        id = parent;
    }
}

void sg::chess::core::GameState::DeleteFrom(const NodeId t_id)
{
    if (t_id == 0 || t_id >= m_nodes.size())
    {
        return;
    }

    // Collect the node and everything after it.
    std::vector<bool> removed(m_nodes.size(), false);
    std::vector<NodeId> pending{ t_id };

    while (!pending.empty())
    {
        const NodeId id{ pending.back() };
        pending.pop_back();

        removed[id] = true;
        pending.insert(pending.end(), m_nodes[id].children.begin(), m_nodes[id].children.end());
    }

    const NodeId parentOfDeleted{ *m_nodes[t_id].parent };
    std::erase(m_nodes[parentOfDeleted].children, t_id);

    // The nodes that stay get new ids. The order is kept, so a parent always comes before its children.
    std::vector<NodeId> newId(m_nodes.size(), 0);
    std::vector<Node> kept;

    for (NodeId id{ 0 }; id < m_nodes.size(); ++id)
    {
        if (!removed[id])
        {
            newId[id] = kept.size();
            kept.push_back(std::move(m_nodes[id]));
        }
    }

    for (Node& node : kept)
    {
        if (node.parent)
        {
            node.parent = newId[*node.parent];
        }

        for (NodeId& child : node.children)
        {
            child = newId[child];
        }
    }

    m_current = removed[m_current] ? newId[parentOfDeleted] : newId[m_current];
    m_nodes = std::move(kept);
}

void sg::chess::core::GameState::Reset() noexcept
{
    m_nodes.resize(1);
    m_nodes[0].children.clear();
    m_nodes[0].note = {};
    m_current = 0;
}

void sg::chess::core::GameState::ReplaceWith(GameState&& t_other) noexcept
{
    m_nodes = std::move(t_other.m_nodes);
    m_current = t_other.m_current;

    // The other game must not be left without a start position.
    t_other.m_nodes.clear();
    t_other.m_current = 0;
}
