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

#include <stdexcept>
#include <vector>
#include "engine/StockfishRules.h"

namespace
{
    // The cache is emptied when it grows too large. Entries are cheap to compute again.
    constexpr std::size_t MAX_CACHE_ENTRIES{ 50000 };

    constexpr std::string_view FEN_PREFIX{ "Fen: " };
    constexpr std::string_view CHECKERS_PREFIX{ "Checkers:" };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::engine::StockfishRules::StockfishRules(Engine& t_engine)
    : m_engine(t_engine)
{
}

//-------------------------------------------------
// RulesProvider
//-------------------------------------------------

sg::chess::core::PositionInfo sg::chess::engine::StockfishRules::Describe(const core::Position& t_position)
{
    return Query(t_position.ToFen(), std::nullopt);
}

sg::chess::core::PositionInfo sg::chess::engine::StockfishRules::Play(const core::Position& t_position, const core::Move& t_move)
{
    return Query(t_position.ToFen(), t_move);
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

sg::chess::core::PositionInfo sg::chess::engine::StockfishRules::Query(const std::string& t_fen,
                                                                       const std::optional<core::Move>& t_move)
{
    const std::string key{ t_fen + '|' + (t_move ? t_move->ToUci() : std::string{}) };

    if (const auto it{ m_cache.find(key) }; it != m_cache.end())
    {
        return it->second;
    }

    m_engine.Send("position fen " + t_fen + (t_move ? " moves " + t_move->ToUci() : std::string{}));

    // Stockfish runs "go perft" in its search thread while it keeps reading commands.
    // So we must wait for the line "Nodes searched", which comes last, before we ask anything else.
    m_engine.Send("go perft 1");    // lists every legal move as "e2e4: 1"
    const std::vector<std::string> perftLines{ m_engine.ReadUntil("Nodes searched") };

    // "d" is answered by the command thread itself, and so is 'isready' right after it.
    // 'readyok' therefore comes after all lines of "d", no matter in which order Stockfish prints them.
    m_engine.Send("d");             // prints "Fen: ..." and "Checkers: ..."
    m_engine.Send("isready");
    const std::vector<std::string> boardLines{ m_engine.ReadUntil("readyok") };

    core::PositionInfo info;
    bool foundFen{ false };

    for (const std::string& line : perftLines)
    {
        // A perft line is "<move>: <count>". Other lines with a colon are not moves and are skipped here.
        if (const auto colon{ line.find(": ") }; colon != std::string::npos)
        {
            if (const auto move{ core::Move::FromUci(std::string_view{ line }.substr(0, colon)) })
            {
                info.legalMoves.push_back(*move);
            }
        }
    }

    for (const std::string& line : boardLines)
    {
        if (line.starts_with(FEN_PREFIX))
        {
            info.position = core::Position::FromFen(std::string_view{ line }.substr(FEN_PREFIX.size()));
            foundFen = true;
        }
        else if (line.starts_with(CHECKERS_PREFIX))
        {
            info.inCheck = line.find_first_not_of(' ', CHECKERS_PREFIX.size()) != std::string::npos;
        }
    }

    if (!foundFen)
    {
        throw std::runtime_error("Unexpected answer from Stockfish: no FEN for position " + t_fen);
    }

    if (m_cache.size() >= MAX_CACHE_ENTRIES)
    {
        m_cache.clear();
    }

    m_cache.emplace(key, info);

    return info;
}
