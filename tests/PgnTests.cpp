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
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Check.h"
#include "FakeRules.h"
#include "core/GameState.h"
#include "core/Pgn.h"
#include "core/Util.h"

namespace
{
    using sg::chess::core::GameState;
    using sg::chess::core::Move;
    using sg::chess::core::PgnHeaders;
    using sg::chess::tests::FakeRules;

    Move Uci(const char* t_uci)
    {
        return *Move::FromUci(t_uci);
    }

    // Everything that PGN can say about the moves, as text: the same text means the same game.
    std::string Dump(const GameState& t_game, const GameState::NodeId t_id)
    {
        const GameState::Node& node{ t_game.At(t_id) };

        std::string text{ node.san + "{" + node.note.comment + "|" + node.note.commands + "|" };
        for (const int nag : node.note.nags)
        {
            text += "$" + std::to_string(nag) + ",";
        }

        text += "|" + node.lineComment + "}";

        for (const GameState::NodeId child : node.children)
        {
            text += "(" + Dump(t_game, child) + ")";
        }

        return text;
    }

    // A game with a main line, two variations, one of them nested, and annotations of every kind.
    void BuildGame(GameState& t_game)
    {
        // 1. e4 e5 2. Nf3 Nc6
        for (const char* move : { "e2e4", "e7e5", "g1f3", "b8c6" })
        {
            CHECK(t_game.Play(Uci(move)));
        }

        // 1. d4 d5 with the alternative 1... Nf6
        t_game.GoToStart();
        CHECK(t_game.Play(Uci("d2d4")));
        const GameState::NodeId d4{ t_game.CurrentId() };
        CHECK(t_game.Play(Uci("d7d5")));
        t_game.GoBack();
        CHECK(t_game.Play(Uci("g8f6")));
        const GameState::NodeId nf6{ t_game.CurrentId() };

        // 1... c5 after 1. e4
        t_game.GoTo(1);
        CHECK(t_game.Play(Uci("c7c5")));
        const GameState::NodeId c5{ t_game.CurrentId() };

        t_game.SetAnnotation(0, { "A test game.", "", {} });
        t_game.SetAnnotation(1, { "The king's pawn.", "[%clk 0:03:00]", { 1 } });
        t_game.SetAnnotation(3, { "", "", { 5 } });
        t_game.SetLineComment(d4, "The queen's pawn. }");
        t_game.SetLineComment(nf6, "The Indian defence.");
        t_game.SetLineComment(c5, "The Sicilian idea.");
        t_game.GoToStart();
    }

    PgnHeaders TestHeaders()
    {
        PgnHeaders headers{ PgnHeaders::MakeDefault("2026.11.20") };
        headers.Set("White", "Me");
        headers.Set("Result", "1-0");
        headers.Set("Annotator", "stwe");

        return headers;
    }

    void TestWrite()
    {
        FakeRules rules;
        GameState game{ rules };
        BuildGame(game);

        const std::string expected{
            "[Event \"Analysis\"]\n"
            "[Site \"?\"]\n"
            "[Date \"2026.11.20\"]\n"
            "[Round \"?\"]\n"
            "[White \"Me\"]\n"
            "[Black \"?\"]\n"
            "[Result \"1-0\"]\n"
            "[Annotator \"stwe\"]\n"
            "\n"
            "{ A test game. } 1. e4 $1 { The king's pawn. [%clk 0:03:00] } ({ The queen's pawn. ) }"
        };

        // The tags and the start of the moves are compared as they are. The rest is covered by the round trip below.
        const std::string text{ sg::chess::core::pgn_write(game, TestHeaders()) };
        CHECK(text.starts_with(expected));
        CHECK(text.ends_with("1-0\n"));
        CHECK(text.find("({ The queen's pawn. ) }") != std::string::npos);

        // Where the lines are broken does not matter here.
        std::string flat{ text };
        std::ranges::replace(flat, '\n', ' ');
        CHECK(flat.find("1... e5 ({ The Sicilian idea. } 1... c5) 2. Nf3 $5 Nc6 1-0") != std::string::npos);

        // The alternative to the reply is written in parentheses, inside the parentheses of the first variation.
        CHECK(flat.find("1. d4 d5 ({ The Indian defence. } 1... Nf6)") != std::string::npos);

        // No line is longer than 80 characters, except those with a comment.
        std::size_t start{ 0 };
        while (start < text.size())
        {
            const std::size_t end{ text.find('\n', start) };
            const std::string line{ text.substr(start, end - start) };
            CHECK(line.size() <= 80 || line.find('{') != std::string::npos);
            start = end + 1;
        }
    }

    void TestRoundTrip()
    {
        FakeRules rules;
        GameState game{ rules };
        BuildGame(game);

        const std::string text{ sg::chess::core::pgn_write(game, TestHeaders()) };
        const auto read{ sg::chess::core::pgn_read(text, rules) };

        // Same moves, same variations in the same order, same comments, commands, glyphs and line comments.
        // The brace in the comment of the first variation became a parenthesis, because PGN cannot write it.
        std::string expected{ Dump(game, 0) };
        expected.replace(expected.find("pawn. }"), 7, "pawn. )");
        CHECK(Dump(read.game, 0) == expected);

        CHECK(read.headers.Get("Annotator") == "stwe");
        CHECK(read.headers.Get("White") == "Me");
        CHECK(read.headers.Get("Result") == "1-0");
        CHECK(read.game.CurrentId() == 0);

        // Written again, the text is the same.
        CHECK(sg::chess::core::pgn_write(read.game, read.headers) == text);
    }

    void TestReadLichessStyle()
    {
        FakeRules rules;

        const std::string text{
            "[Event \"Rated Blitz game\"]\n"
            "[Site \"https://lichess.org/abcd1234\"]\n"
            "[White \"alice\"]\n"
            "[Black \"bob\"]\n"
            "[Result \"1-0\"]\n"
            "\n"
            "1.e4 { [%clk 0:03:00] } e5 { [%eval 0.3] [%clk 0:02:59] A comment.\nOn two lines. }\n"
            "2.Nf3! Nc6?! ; the rest of the line\n"
            "1-0\n"
            "\n"
            "[Event \"The second game is ignored\"]\n"
            "\n"
            "1. d4 *\n"
        };

        const auto read{ sg::chess::core::pgn_read(text, rules) };
        const GameState& game{ read.game };

        CHECK(read.headers.Get("Event") == "Rated Blitz game");
        CHECK(read.headers.Get("Site") == "https://lichess.org/abcd1234");
        CHECK(game.NodeCount() == 5);

        // The commands are kept apart from the text.
        CHECK(game.At(1).san == "e4");
        CHECK(game.At(1).note.comment.empty());
        CHECK(game.At(1).note.commands == "[%clk 0:03:00]");
        CHECK(game.At(2).note.commands == "[%eval 0.3] [%clk 0:02:59]");
        CHECK(game.At(2).note.comment == "A comment.\nOn two lines.");

        // The glyphs after the moves are numbers, the comment after a semicolon is a comment.
        CHECK(game.At(3).note.nags == std::vector<int>{ 1 });
        CHECK(game.At(4).note.nags == std::vector<int>{ 6 });
        CHECK(game.At(4).note.comment == "the rest of the line");
    }

    void TestReadVariations()
    {
        FakeRules rules;

        // A comment before the first move of a variation is the comment on the variation.
        const auto read{ sg::chess::core::pgn_read(
            "1. e4 ( { The queen's pawn. [%clk 0:01:00] } 1. d4 { Calm. } d5 $2 ) e5 { Main. } 2. Nf3 *", rules) };
        const GameState& game{ read.game };

        CHECK(game.At(0).children.size() == 2);

        const GameState::Node& d4{ game.At(game.At(0).children[1]) };
        CHECK(d4.san == "d4");
        CHECK(d4.lineComment == "The queen's pawn.");
        CHECK(d4.note.commands == "[%clk 0:01:00]");
        CHECK(d4.note.comment == "Calm.");
        CHECK(game.At(d4.children[0]).note.nags == std::vector<int>{ 2 });

        // The main line goes on after the variation.
        CHECK(game.At(1).san == "e4");
        CHECK(game.At(game.At(1).children[0]).note.comment == "Main.");
        CHECK(game.At(game.At(game.At(1).children[0]).children[0]).san == "Nf3");
    }

    void TestSetupFen()
    {
        FakeRules rules;

        const std::string fen{ sg::chess::tests::AFTER_E4 };
        const auto read{ sg::chess::core::pgn_read(
            "[SetUp \"1\"]\n[FEN \"" + fen + "\"]\n\n1... e5 2. Nf3 *\n", rules) };

        CHECK(read.game.At(0).info.position.ToFen() == sg::chess::core::Position::FromFen(fen).ToFen());
        CHECK(read.game.NodeCount() == 3);
        CHECK(read.game.At(1).san == "e5");

        // A game from another position says so when it is written: the tags and the number of Black's first move.
        const std::string text{ sg::chess::core::pgn_write(read.game, read.headers) };
        CHECK(text.find("[SetUp \"1\"]\n[FEN \"" + sg::chess::core::Position::FromFen(fen).ToFen() + "\"]") !=
              std::string::npos);
        CHECK(text.find("\n1... e5 2. Nf3 *\n") != std::string::npos);

        const auto again{ sg::chess::core::pgn_read(text, rules) };
        CHECK(Dump(again.game, 0) == Dump(read.game, 0));
    }

    void TestOddFiles()
    {
        FakeRules rules;

        // A byte order mark, a line for programs and a game without tags.
        const auto read{ sg::chess::core::pgn_read("\xEF\xBB\xBF% for programs only\n1. e4 e5 1/2-1/2", rules) };

        CHECK(read.game.NodeCount() == 3);
        CHECK(read.headers.Get("Result") == "1/2-1/2");

        // No moves at all is a valid game.
        const auto empty{ sg::chess::core::pgn_read("[Event \"Nothing\"]\n\n*\n", rules) };
        CHECK(!empty.game.HasMoves());
    }

    // The text of the error, or an empty text if the PGN was read without an error.
    std::string ErrorOf(const std::string& t_pgn)
    {
        FakeRules rules;

        try
        {
            static_cast<void>(sg::chess::core::pgn_read(t_pgn, rules));
        }
        catch (const std::runtime_error& t_error)
        {
            return t_error.what();
        }

        return "";
    }

    void TestPlayers()
    {
        using sg::chess::core::Color;

        PgnHeaders headers{ PgnHeaders::MakeDefault("2026.11.20") };

        // Nothing known: the sides are named by their color, and there is no rating.
        CHECK(headers.PlayerName(Color::WHITE) == "White");
        CHECK(headers.PlayerName(Color::BLACK) == "Black");
        CHECK(headers.PlayerRating(Color::WHITE).empty());

        headers.Set("White", "  Carlsen, Magnus ");
        headers.Set("WhiteElo", "2882");
        headers.Set("BlackElo", "?");
        CHECK(headers.PlayerName(Color::WHITE) == "Carlsen, Magnus");
        CHECK(headers.PlayerRating(Color::WHITE) == "2882");
        CHECK(headers.PlayerRating(Color::BLACK).empty());

        headers.Remove("WhiteElo");
        headers.Remove("Nothing");
        CHECK(headers.PlayerRating(Color::WHITE).empty());
        CHECK(headers.Get("WhiteElo").empty());
    }

    void TestDate()
    {
        // "2026.11.20": four digits, a dot, two digits, a dot, two digits.
        const std::string date{ sg::chess::core::format_local_time("%Y.%m.%d") };

        CHECK(date.size() == 10);
        CHECK(date[4] == '.' && date[7] == '.');
        CHECK(sg::chess::core::PgnHeaders::MakeDefault(date).Get("Date") == date);
    }

    void TestStandardTags()
    {
        FakeRules rules;
        GameState game{ rules };
        CHECK(game.Play(Uci("e2e4")));

        // The tags of the standard that are not in the seven tag roster, with a time control in seconds.
        PgnHeaders headers{ PgnHeaders::MakeDefault("2026.11.24") };
        headers.Set("TimeControl", "900+10");
        headers.Set("ECO", "B90");
        headers.Set("Opening", "Sicilian Defense");
        headers.Set("Variation", "Najdorf Variation");
        headers.Set("White", "Carlsen, Magnus");
        headers.Set("WhiteElo", "2882");

        const std::string text{ sg::chess::core::pgn_write(game, headers) };

        // The seven tags come first and in their order, the others follow.
        CHECK(text.starts_with("[Event \"Analysis\"]\n[Site \"?\"]\n[Date \"2026.11.24\"]\n[Round \"?\"]\n"
                               "[White \"Carlsen, Magnus\"]\n[Black \"?\"]\n[Result \"*\"]\n"));
        CHECK(text.find("[TimeControl \"900+10\"]\n") != std::string::npos);
        CHECK(text.find("[ECO \"B90\"]\n") != std::string::npos);
        CHECK(text.find("[Variation \"Najdorf Variation\"]\n") != std::string::npos);
        CHECK(text.find("[Result") < text.find("[TimeControl"));

        // They come back from the file.
        FakeRules readRules;
        const auto read{ sg::chess::core::pgn_read(text, readRules) };
        CHECK(read.headers.Get("TimeControl") == "900+10");
        CHECK(read.headers.Get("ECO") == "B90");
        CHECK(read.headers.Get("Opening") == "Sicilian Defense");
        CHECK(read.headers.Get("Variation") == "Najdorf Variation");
        CHECK(read.headers.Get("Date") == "2026.11.24");
    }

    void TestErrors()
    {
        // The messages tell what is wrong and where.
        CHECK(ErrorOf("1. e5 *").find("line 1") != std::string::npos);
        CHECK(ErrorOf("1. e5 *").find("\"e5\"") != std::string::npos);
        CHECK(ErrorOf("[Event \"x\"]\n\n1. e4 e5 2. Qh5 *").find("line 3") != std::string::npos);
        CHECK(!ErrorOf("1. e4 ) *").empty());
        CHECK(!ErrorOf("1. e4 ( 1. d4 d5").empty());
        CHECK(!ErrorOf("1. e4 { never closed").empty());
        CHECK(!ErrorOf("( 1. e4 ) *").empty());
        CHECK(!ErrorOf("1. e4 $ *").empty());
        CHECK(!ErrorOf("[FEN \"nonsense\"]\n\n*").empty());

        // A FEN that is written correctly but cannot be played is refused as well: here there is no Black king.
        CHECK(ErrorOf("[FEN \"8/8/8/8/8/8/8/4K3 w - - 0 1\"]\n\n*").find("playable") != std::string::npos);
        CHECK(!ErrorOf("[White \"unfinished]\n\n*").empty());

        // A correct game gives no error.
        CHECK(ErrorOf("1. e4 e5 *").empty());
    }
}

int main()
{
    TestWrite();
    TestRoundTrip();
    TestReadLichessStyle();
    TestReadVariations();
    TestSetupFen();
    TestOddFiles();
    TestPlayers();
    TestDate();
    TestStandardTags();
    TestErrors();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
