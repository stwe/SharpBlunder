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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include "Check.h"
#include "core/ChessComParser.h"
#include "core/GameSummary.h"

namespace
{
    namespace core = sg::chess::core;

    // The recorded answers of chess.com (opponents are made anonymous), see tests/data.
    std::string ReadData(const char* t_name)
    {
        std::ifstream file{ std::filesystem::path{ SHARPBLUNDER_TEST_DATA_DIR } / t_name, std::ios::binary };

        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }

    // A game of the answer, as small as it can be. The PGN has no moves that count.
    std::string Game(const std::string& t_rules, const std::string& t_pgn, const std::string& t_uuid = "u1")
    {
        return "{\"url\":\"https://www.chess.com/game/live/1\",\"pgn\":\"" + t_pgn + "\",\"uuid\":\"" + t_uuid
               + "\",\"time_control\":\"180\",\"time_class\":\"blitz\",\"rules\":\"" + t_rules
               + "\",\"end_time\":1700000000,\"rated\":false,"
                 "\"white\":{\"rating\":1000,\"result\":\"win\",\"username\":\"Me\"},"
                 "\"black\":{\"rating\":900,\"result\":\"timeout\",\"username\":\"You\"}}";
    }

    void TestArchives()
    {
        const auto archives{ core::parse_chesscom_archives(ReadData("chesscom_archives.json")) };

        CHECK(archives.has_value());
        CHECK(archives->size() == 8);
        CHECK(archives->front() == "https://api.chess.com/pub/player/steffengo/games/2026/04");
        CHECK(archives->back() == "https://api.chess.com/pub/player/steffengo/games/2026/11");

        // No archives yet: an empty list is an answer, too. Everything else is not.
        CHECK(core::parse_chesscom_archives("{\"archives\":[]}")->empty());
        CHECK(!core::parse_chesscom_archives("not json"));
        CHECK(!core::parse_chesscom_archives("{\"games\":[]}"));
        CHECK(!core::parse_chesscom_archives("{\"archives\":3}"));
        CHECK(core::parse_chesscom_archives("{\"archives\":[1,\"a\"]}")->size() == 1);
    }

    void TestMonth()
    {
        const auto month{ core::parse_chesscom_month(ReadData("chesscom_2026-11.json"), "steffengo") };

        CHECK(month.has_value());
        CHECK(month->games.size() == 8);
        CHECK(month->skippedVariants == 0);
        CHECK(month->skippedBroken == 0);

        // The first game: the user was White and was checkmated.
        const core::GameSummary& first{ month->games[0] };
        CHECK(first.source == "chess.com");
        CHECK(first.id == "5a728cc3-dd4e-11f1-82c6-fd6c6a01000f");
        CHECK(first.url == "https://www.chess.com/game/live/186943700414");
        CHECK(first.white == "SteffenGo");
        CHECK(first.black == "Opponent1");
        CHECK(first.whiteRating == 350);
        CHECK(first.blackRating == 391);
        CHECK(first.userColor == core::Color::WHITE);
        CHECK(first.result == "0-1");
        CHECK(first.outcome == core::Outcome::LOSS);
        CHECK(first.endReason == "checkmated");
        CHECK(first.timeControl == "900+10");
        CHECK(first.timeClass == "rapid");
        CHECK(first.eco == "D00");
        CHECK(first.opening == "Queens Pawn Opening Blackmar Blackmar Diemer Gambit");
        CHECK(first.moves == 52);
        CHECK(first.rated);
        CHECK(first.whiteAccuracy && *first.whiteAccuracy > 71.16 && *first.whiteAccuracy < 71.18);
        CHECK(first.blackAccuracy && *first.blackAccuracy > 73.18 && *first.blackAccuracy < 73.20);
        CHECK(core::format_game_date(first.endTime) == "2026-11-10");
        CHECK(first.pgn.starts_with("[Event \"Live Chess\"]"));

        // The user as Black and the winner.
        const core::GameSummary& second{ month->games[1] };
        CHECK(second.userColor == core::Color::BLACK);
        CHECK(second.outcome == core::Outcome::LOSS);
        const core::GameSummary& fifth{ month->games[4] };
        CHECK(fifth.userColor == core::Color::BLACK);
        CHECK(fifth.result == "0-1");
        CHECK(fifth.outcome == core::Outcome::WIN);
        CHECK(fifth.endReason == "resigned");
        CHECK(fifth.moves == 26);

        // Games without accuracies. A win because the other player left.
        CHECK(!month->games[2].whiteAccuracy);
        CHECK(month->games[5].endReason == "abandoned");
        CHECK(month->games[5].outcome == core::Outcome::WIN);
        CHECK(month->games[5].moves == 6);

        // The name of the user may be written in any case.
        const auto lower{ core::parse_chesscom_month(ReadData("chesscom_2026-11.json"), "STEFFENGO") };
        CHECK(lower->games[0].userColor == core::Color::WHITE);

        // Another user: nobody's point of view.
        const auto other{ core::parse_chesscom_month(ReadData("chesscom_2026-11.json"), "somebody") };
        CHECK(!other->games[0].userColor);
        CHECK(other->games[0].outcome == core::Outcome::UNKNOWN);
        CHECK(other->games[0].result == "0-1");
    }

    void TestSkippedAndBroken()
    {
        const std::string pgn{ "[Event \\\"x\\\"]\\n\\n1. e4 e5 1-0" };

        // Variants are skipped and counted. Games without moves, and entries that are no games, are broken.
        const std::string json{ "{\"games\":[" + Game("chess", pgn) + "," + Game("chess960", pgn, "u2") + ","
                                + Game("chess", "", "u3") + ",3,{}]}" };
        const auto month{ core::parse_chesscom_month(json, "me") };

        CHECK(month.has_value());
        CHECK(month->games.size() == 1);
        CHECK(month->skippedVariants == 1);
        CHECK(month->skippedBroken == 3);
        CHECK(month->games[0].id == "u1");
        CHECK(month->games[0].outcome == core::Outcome::WIN);

        // A game without an uuid can still be told apart by its address.
        const std::string noId{ "{\"games\":[" + Game("chess", pgn, "") + "]}" };
        CHECK(core::parse_chesscom_month(noId, "me")->games[0].id == "https://www.chess.com/game/live/1");

        // No games: an empty month is fine. What is not an answer of this kind is not.
        CHECK(core::parse_chesscom_month("{\"games\":[]}", "me")->games.empty());
        CHECK(!core::parse_chesscom_month("", "me"));
        CHECK(!core::parse_chesscom_month("{\"games\":", "me"));
        CHECK(!core::parse_chesscom_month("{\"archives\":[]}", "me"));
    }

    void TestUsername()
    {
        CHECK(core::is_valid_chesscom_username("SteffenGo"));
        CHECK(core::is_valid_chesscom_username("a_b-c9"));
        CHECK(!core::is_valid_chesscom_username(""));
        CHECK(!core::is_valid_chesscom_username("a b"));
        CHECK(!core::is_valid_chesscom_username("a/b"));
        CHECK(!core::is_valid_chesscom_username("../x"));
        CHECK(!core::is_valid_chesscom_username("a?b=c"));
        CHECK(!core::is_valid_chesscom_username("\xC3\xA4"));
        CHECK(!core::is_valid_chesscom_username(std::string(51, 'a')));
    }

    void TestResults()
    {
        CHECK(core::classify_result("win") == core::Outcome::WIN);
        CHECK(core::classify_result("checkmated") == core::Outcome::LOSS);
        CHECK(core::classify_result("timeout") == core::Outcome::LOSS);
        CHECK(core::classify_result("resigned") == core::Outcome::LOSS);
        CHECK(core::classify_result("abandoned") == core::Outcome::LOSS);
        CHECK(core::classify_result("agreed") == core::Outcome::DRAW);
        CHECK(core::classify_result("repetition") == core::Outcome::DRAW);
        CHECK(core::classify_result("stalemate") == core::Outcome::DRAW);
        CHECK(core::classify_result("50move") == core::Outcome::DRAW);
        CHECK(core::classify_result("something new") == core::Outcome::UNKNOWN);
        CHECK(core::classify_result("") == core::Outcome::UNKNOWN);

        CHECK(core::pgn_result("win", "resigned") == "1-0");
        CHECK(core::pgn_result("timeout", "win") == "0-1");
        CHECK(core::pgn_result("agreed", "agreed") == "1/2-1/2");
        CHECK(core::pgn_result("insufficient", "timevsinsufficient") == "1/2-1/2");
        CHECK(core::pgn_result("", "") == "*");
        CHECK(core::pgn_result("win", "win") == "*");

        CHECK(core::outcome_for("1-0", core::Color::WHITE) == core::Outcome::WIN);
        CHECK(core::outcome_for("1-0", core::Color::BLACK) == core::Outcome::LOSS);
        CHECK(core::outcome_for("0-1", core::Color::BLACK) == core::Outcome::WIN);
        CHECK(core::outcome_for("1/2-1/2", core::Color::BLACK) == core::Outcome::DRAW);
        CHECK(core::outcome_for("*", core::Color::WHITE) == core::Outcome::UNKNOWN);

        // The language of the tests is English. Codes that are not known stay as they are.
        CHECK(core::describe_end_reason("checkmated") == "Checkmate");
        CHECK(core::describe_end_reason("resigned") == "Resignation");
        CHECK(core::describe_end_reason("50move") == "50-move rule");
        CHECK(core::describe_end_reason("surprise") == "surprise");
    }

    void TestTexts()
    {
        CHECK(core::short_time_control("900+10") == "15+10");
        CHECK(core::short_time_control("180+2") == "3+2");
        CHECK(core::short_time_control("600") == "10 min");
        CHECK(core::short_time_control("30") == "30 s");
        CHECK(core::short_time_control("90+0") == "1.5 min");
        CHECK(core::short_time_control("1/86400") == "1 d");
        CHECK(core::short_time_control("1/259200") == "3 d");
        CHECK(core::short_time_control("?") == "?");
        CHECK(core::short_time_control("") == "");

        CHECK(core::format_game_date(0).empty());
        CHECK(core::format_game_date(-5).empty());
        CHECK(core::format_game_date(1700000000) == "2023-11-14");
        CHECK(core::format_game_date(86399) == "1970-01-01");

        CHECK(core::opening_from_url("https://www.chess.com/openings/Sicilian-Defense-Najdorf-Variation")
              == "Sicilian Defense Najdorf Variation");
        CHECK(core::opening_from_url("https://www.chess.com/openings/French-Defense?x=1") == "French Defense");
        CHECK(core::opening_from_url("").empty());
    }

    void TestCountMoves()
    {
        // Only move numbers count: not the result, not the numbers in comments or in tags.
        CHECK(core::count_moves("[Round \"9.\"]\n\n1. e4 e5 2. Nf3 Nc6 1-0") == 2);
        CHECK(core::count_moves("1. e4 {99. no} e5 2. d4 {[%clk 0:09:58.7]} 2... exd4 1/2-1/2") == 2);
        CHECK(core::count_moves("1. e4 e5 2. Nf3 {a {nested} 7. b} Nc6") == 2);
        CHECK(core::count_moves("1.e4 e5 2.Nf3") == 2);
        CHECK(core::count_moves("[Event \"x\"]\n\n*") == 0);
        CHECK(core::count_moves("") == 0);
    }
} // namespace

int main()
{
    TestArchives();
    TestMonth();
    TestSkippedAndBroken();
    TestUsername();
    TestResults();
    TestTexts();
    TestCountMoves();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
