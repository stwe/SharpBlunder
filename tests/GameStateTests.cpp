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
#include <utility>
#include <vector>
#include "Check.h"
#include "FakeRules.h"
#include "core/GameState.h"
#include "core/Move.h"

namespace
{
    using sg::chess::core::GameState;
    using sg::chess::core::Move;
    using sg::chess::tests::FakeRules;

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

    void TestAnnotations()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        const GameState::NodeId e4{ game.CurrentId() };

        game.SetAnnotation(e4, { "A good move.", "[%clk 0:03:00]", { 1 } });
        CHECK(game.At(e4).note.comment == "A good move.");
        CHECK(game.At(e4).note.commands == "[%clk 0:03:00]");
        CHECK(game.At(e4).note.nags == std::vector<int>{ 1 });
        CHECK(!game.At(e4).note.IsEmpty());

        // The comment on the game is the annotation of the start position.
        game.SetAnnotation(0, { "About the whole game.", "", {} });
        CHECK(game.At(0).note.comment == "About the whole game.");

        // An id that does not exist is ignored.
        game.SetAnnotation(99, { "nowhere", "", {} });
        game.SetLineComment(99, "nowhere");
        CHECK(game.NodeCount() == 2);

        // Reset removes the comment on the game as well.
        game.Reset();
        CHECK(game.At(0).note.IsEmpty());
    }

    void TestLineCommentAndPromote()
    {
        FakeRules rules;
        GameState game{ rules };

        CHECK(game.Play(Uci("e2e4")));
        game.GoToStart();
        CHECK(game.Play(Uci("d2d4")));

        const GameState::NodeId d4{ game.CurrentId() };
        game.SetLineComment(d4, "The queen's pawn.");
        game.SetAnnotation(d4, { "Solid.", "", {} });
        CHECK(game.At(d4).lineComment == "The queen's pawn.");

        // A variation that becomes the main line has no line comment any more. It is kept in front of the move comment.
        game.Promote(d4);
        CHECK(game.At(d4).lineComment.empty());
        CHECK(game.At(d4).note.comment == "The queen's pawn.\nSolid.");

        // The line that was the main line is a variation now, without a line comment.
        const GameState::NodeId e4{ game.At(0).children[1] };
        CHECK(game.At(e4).san == "e4");
        CHECK(game.At(e4).lineComment.empty());
    }

    void TestReplaceWith()
    {
        FakeRules rules;
        GameState first{ rules };
        GameState second{ rules };

        CHECK(second.Play(Uci("d2d4")));
        second.SetAnnotation(1, { "Loaded.", "", {} });
        second.GoToStart();

        first.ReplaceWith(std::move(second));
        CHECK(first.HasMoves());
        CHECK(first.CurrentId() == 0);
        CHECK(first.At(1).san == "d4");
        CHECK(first.At(1).note.comment == "Loaded.");
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
    TestAnnotations();
    TestLineCommentAndPromote();
    TestReplaceWith();
    TestReset();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
