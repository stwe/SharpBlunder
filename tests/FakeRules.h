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

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"
#include "core/RulesProvider.h"

namespace sg::chess::tests
{
    using core::Move;
    using core::Position;
    using core::PositionInfo;

    constexpr const char* AFTER_E4{ "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1" };
    constexpr const char* AFTER_E4_E5{ "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2" };
    constexpr const char* AFTER_E4_E5_NF3{ "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2" };
    constexpr const char* AFTER_E4_E5_NF3_NC6{ "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3" };
    constexpr const char* AFTER_D4_D5{ "rnbqkbnr/ppp1pppp/8/3p4/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 0 2" };
    constexpr const char* AFTER_E4_C5{ "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2" };
    constexpr const char* AFTER_D4{ "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1" };

    /**
     * @brief Rules for a handful of opening moves, so the tests need no Stockfish.
     *
     * It knows exactly the positions and moves listed in the constructor. Everything else has no legal move.
     */
    class FakeRules final : public core::RulesProvider
    {
    public:
        FakeRules()
        {
            Add(Position::START_FEN, "e2e4", AFTER_E4);
            Add(Position::START_FEN, "d2d4", AFTER_D4);
            Add(AFTER_E4, "e7e5", AFTER_E4_E5);
            Add(AFTER_E4, "c7c5", AFTER_E4_C5);
            Add(AFTER_E4_E5, "g1f3", AFTER_E4_E5_NF3);
            Add(AFTER_E4_E5_NF3, "b8c6", AFTER_E4_E5_NF3_NC6);
            Add(AFTER_D4, "d7d5", AFTER_D4_D5);
        }

        [[nodiscard]] PositionInfo Describe(const Position& t_position) override
        {
            PositionInfo info;
            info.position = t_position;

            for (const auto& [key, next] : m_next)
            {
                if (key.first == t_position.ToFen())
                {
                    info.legalMoves.push_back(*Move::FromUci(key.second));
                }
            }

            return info;
        }

        [[nodiscard]] PositionInfo Play(const Position& t_position, const Move& t_move) override
        {
            return Describe(Position::FromFen(m_next.at({ t_position.ToFen(), t_move.ToUci() })));
        }

    private:
        void Add(const std::string_view t_from, const std::string& t_uci, const std::string_view t_to)
        {
            m_next[{ Position::FromFen(t_from).ToFen(), t_uci }] = Position::FromFen(t_to).ToFen();
        }

        std::map<std::pair<std::string, std::string>, std::string> m_next;
    };
}
