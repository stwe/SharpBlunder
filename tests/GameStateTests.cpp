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

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "Check.h"
#include "core/GameState.h"
#include "core/Move.h"
#include "core/RulesProvider.h"

namespace
{
    using sg::chess::core::GameState;
    using sg::chess::core::Move;
    using sg::chess::core::Position;
    using sg::chess::core::PositionInfo;

    constexpr const char* AFTER_E4{ "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1" };
    constexpr const char* AFTER_E4_E5{ "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2" };
    constexpr const char* AFTER_E4_E5_NF3{ "rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2" };
    constexpr const char* AFTER_E4_C5{ "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2" };
    constexpr const char* AFTER_D4{ "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1" };

    /**
     * @brief Rules for a handful of opening moves, so the tests need no Stockfish.
     *
     * It knows exactly the positions and moves listed in the constructor. Everything else has no legal move.
     */
    class FakeRules final : public sg::chess::core::RulesProvider
    {
    public:
        FakeRules()
        {
            Add(Position::START_FEN, "e2e4", AFTER_E4);
            Add(Position::START_FEN, "d2d4", AFTER_D4);
            Add(AFTER_E4, "e7e5", AFTER_E4_E5);
            Add(AFTER_E4, "c7c5", AFTER_E4_C5);
            Add(AFTER_E4_E5, "g1f3", AFTER_E4_E5_NF3);
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

    Move Uci(const char* t_uci)
    {
        return *Move::FromUci(t_uci);
    }

    void TestMainLine()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(!game.HasMoves());
        CHECK(game.Play(Uci("e2e4")));
        CHECK(game.Play(Uci("e7e5")));
        CHECK(game.HasMoves());
        CHECK(game.Current().san == "e5");
        CHECK(game.CurrentId() == 2);

        // An illegal move is refused and changes nothing.
        CHECK(!game.Play(Uci("a2a4")));
        CHECK(game.CurrentId() == 2);

        game.GoBack();
        CHECK(game.Current().san == "e4");
        game.GoToStart();
        CHECK(!game.CanGoBack());
        game.GoToEnd();
        CHECK(game.CurrentId() == 2);
    }

    void TestVariation()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        CHECK(game.Play(Uci("e7e5")));
        game.GoToStart();

        // Another move in the start position: a variation, nothing is thrown away.
        CHECK(game.Play(Uci("d2d4")));
        CHECK(game.Current().san == "d4");
        CHECK(game.At(0).children.size() == 2);
        CHECK(game.At(game.At(0).children[0]).san == "e4");
        CHECK(game.At(game.At(0).children[1]).san == "d4");
        CHECK(!game.IsMainLine(game.CurrentId()));

        // The main line is still complete.
        game.GoToStart();
        game.GoToEnd();
        CHECK(game.Current().san == "e5");
        CHECK(game.IsMainLine(game.CurrentId()));

        // The same move again does not add a node: the game goes to the move that is there.
        const std::size_t before{ game.At(0).children.size() };
        game.GoToStart();
        CHECK(game.Play(Uci("e2e4")));
        CHECK(game.At(0).children.size() == before);
        CHECK(game.Current().san == "e4");
        CHECK(game.CurrentId() == 1);

        // A variation deeper in the game.
        CHECK(game.Play(Uci("e7e5")));
        game.GoBack();
        CHECK(game.Play(Uci("c7c5")));
        CHECK(game.At(1).children.size() == 2);
        CHECK(game.Current().san == "c5");
    }

    void TestGoForwardFollowsTheMainLine()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        game.GoToStart();
        CHECK(game.Play(Uci("d2d4")));
        game.GoToStart();

        game.GoForward();
        CHECK(game.Current().san == "e4");
    }

    void TestPromote()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        CHECK(game.Play(Uci("e7e5")));
        game.GoBack();
        CHECK(game.Play(Uci("c7c5")));

        const GameState::NodeId c5{ game.CurrentId() };
        CHECK(!game.IsMainLine(c5));

        game.Promote(c5);
        CHECK(game.IsMainLine(c5));
        CHECK(game.CurrentId() == c5);

        // The old main line is a variation now.
        game.GoToStart();
        game.GoToEnd();
        CHECK(game.Current().san == "c5");

        // Promoting a node of the main line, or the start position, changes nothing.
        game.Promote(c5);
        game.Promote(0);
        CHECK(game.IsMainLine(c5));
    }

    void TestDelete()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        CHECK(game.Play(Uci("e7e5")));
        CHECK(game.Play(Uci("g1f3")));
        game.GoToStart();
        CHECK(game.Play(Uci("d2d4")));

        // Nodes: 0 start, 1 e4, 2 e5, 3 Nf3, 4 d4. Deleting e5 removes e5 and Nf3, d4 gets a new id.
        game.GoTo(3);
        game.DeleteFrom(2);

        CHECK(game.HasMoves());
        CHECK(game.Current().san == "e4");
        CHECK(game.At(1).children.empty());
        CHECK(game.At(0).children.size() == 2);
        CHECK(game.At(game.At(0).children[1]).san == "d4");
        CHECK(game.At(game.At(0).children[1]).parent == 0);

        // The current position outside of the deleted nodes stays the same one, whatever its new id is.
        game.GoTo(game.At(0).children[1]);
        game.DeleteFrom(1);
        CHECK(game.Current().san == "d4");
        CHECK(game.At(0).children.size() == 1);

        // The start position cannot be deleted.
        game.DeleteFrom(0);
        CHECK(game.HasMoves());
    }

    void TestReset()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        game.GoToStart();
        CHECK(game.Play(Uci("d2d4")));
        game.Reset();

        CHECK(!game.HasMoves());
        CHECK(game.CurrentId() == 0);
        CHECK(game.At(0).children.empty());
        CHECK(game.Play(Uci("e2e4")));
    }
}

int main()
{
    TestMainLine();
    TestVariation();
    TestGoForwardFollowsTheMainLine();
    TestPromote();
    TestDelete();
    TestReset();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
