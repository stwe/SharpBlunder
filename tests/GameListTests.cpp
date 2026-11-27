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
#include <string>
#include <vector>
#include "Check.h"
#include "core/GameList.h"

namespace
{
    namespace core = sg::chess::core;

    core::GameSummary Make(const std::string& t_id, const std::int64_t t_time, const std::string& t_class,
                           const std::string& t_white = "Me", const std::string& t_black = "You")
    {
        core::GameSummary game;
        game.source = "chess.com";
        game.id = t_id;
        game.endTime = t_time;
        game.timeClass = t_class;
        game.white = t_white;
        game.black = t_black;

        return game;
    }

    void TestSort()
    {
        std::vector<core::GameSummary> games{ Make("a", 100, "blitz"), Make("c", 300, "blitz"),
                                              Make("b", 200, "blitz"), Make("d", 300, "blitz") };
        core::sort_newest_first(games);

        // The newest first. Games at the same time: by id.
        CHECK(games[0].id == "c");
        CHECK(games[1].id == "d");
        CHECK(games[2].id == "b");
        CHECK(games[3].id == "a");
    }

    void TestAdd()
    {
        std::vector<core::GameSummary> list;

        core::add_games(list, { Make("a", 100, "blitz"), Make("b", 200, "rapid") });
        CHECK(list.size() == 2);
        CHECK(list[0].id == "b");

        // The same game again replaces the old data. A new game is added and sorted.
        core::GameSummary changed{ Make("a", 100, "blitz") };
        changed.whiteAccuracy = 80.0;
        core::add_games(list, { changed, Make("c", 300, "bullet") });
        CHECK(list.size() == 3);
        CHECK(list[0].id == "c");
        CHECK(list[2].id == "a");
        CHECK(list[2].whiteAccuracy.has_value());

        // The same id from another source is another game.
        core::GameSummary other{ Make("a", 50, "blitz") };
        other.source = "lichess.org";
        core::add_games(list, { other });
        CHECK(list.size() == 4);

        core::add_games(list, {});
        CHECK(list.size() == 4);
    }

    void TestFilter()
    {
        core::GameSummary one{ Make("1", 1, "rapid", "SteffenGo", "Magnus") };
        one.opening = "Sicilian Defense Najdorf Variation";
        one.eco = "B90";
        core::GameSummary two{ Make("2", 2, "blitz", "Hikaru", "SteffenGo") };
        two.opening = "French Defense";
        two.eco = "C02";
        const std::vector<core::GameSummary> games{ one, two, Make("3", 3, "rapid") };

        // No filter: everything.
        CHECK(core::filter_games(games, {}).size() == 3);

        // Time class.
        CHECK(core::filter_games(games, { "rapid", "" }) == (std::vector<std::size_t>{ 0, 2 }));
        CHECK(core::filter_games(games, { "blitz", "" }) == (std::vector<std::size_t>{ 1 }));
        CHECK(core::filter_games(games, { "daily", "" }).empty());

        // Text: names, opening and ECO code, in any case.
        CHECK(core::filter_games(games, { "", "magnus" }) == (std::vector<std::size_t>{ 0 }));
        CHECK(core::filter_games(games, { "", "STEFFEN" }) == (std::vector<std::size_t>{ 0, 1 }));
        CHECK(core::filter_games(games, { "", "najdorf" }) == (std::vector<std::size_t>{ 0 }));
        CHECK(core::filter_games(games, { "", "c02" }) == (std::vector<std::size_t>{ 1 }));
        CHECK(core::filter_games(games, { "", "nothing" }).empty());

        // Both together.
        CHECK(core::filter_games(games, { "blitz", "steffen" }) == (std::vector<std::size_t>{ 1 }));
        CHECK(core::filter_games(games, { "rapid", "hikaru" }).empty());

        CHECK(core::filter_games({}, {}).empty());
    }

    void TestTimeClasses()
    {
        const std::vector<core::GameSummary> games{ Make("1", 1, "rapid"), Make("2", 2, "bullet"),
                                                    Make("3", 3, "rapid"), Make("4", 4, ""), Make("5", 5, "odd") };

        // In the usual order, once each, and only what there is.
        CHECK(core::time_classes(games) == (std::vector<std::string>{ "bullet", "rapid" }));
        CHECK(core::time_classes({}).empty());
    }
    // A game for the tests of sorting: the user is White or Black, and plays against "opponent".
    core::GameSummary Played(const std::string& t_id, const std::int64_t t_time, const core::Color t_color,
                             const core::Outcome t_outcome, const std::string& t_opponent,
                             const std::string& t_timeControl, const int t_moves)
    {
        core::GameSummary game{ Make(t_id, t_time, "rapid") };
        game.userColor = t_color;
        game.outcome = t_outcome;
        (t_color == core::Color::WHITE ? game.black : game.white) = t_opponent;
        (t_color == core::Color::WHITE ? game.white : game.black) = "Me";
        game.timeControl = t_timeControl;
        game.moves = t_moves;

        return game;
    }

    // The ids in the order that the keys give.
    std::string Order(const std::vector<core::GameSummary>& t_games, const std::vector<core::SortKey>& t_keys)
    {
        std::vector<std::size_t> indexes;

        for (std::size_t i{ 0 }; i < t_games.size(); ++i)
        {
            indexes.push_back(i);
        }

        core::sort_games(t_games, indexes, t_keys);

        std::string ids;

        for (const std::size_t index : indexes)
        {
            ids += t_games[index].id;
        }

        return ids;
    }

    void TestSortColumns()
    {
        using core::Color;
        using core::Outcome;
        using core::SortColumn;

        // a: oldest. b: Black, a loss, a long game. c: draw, a daily game. d: newest, a win with an increment.
        std::vector<core::GameSummary> games{
            Played("a", 100, Color::WHITE, Outcome::WIN, "bob", "600", 30),
            Played("b", 200, Color::BLACK, Outcome::LOSS, "Alice", "900+10", 60),
            Played("c", 300, Color::WHITE, Outcome::DRAW, "carl", "1/86400", 10),
            Played("d", 400, Color::BLACK, Outcome::WIN, "Dora", "180+2", 20),
        };

        // Without keys: the newest first.
        CHECK(Order(games, {}) == "dcba");

        // Date, both ways.
        CHECK(Order(games, { { SortColumn::DATE, true } }) == "abcd");
        CHECK(Order(games, { { SortColumn::DATE, false } }) == "dcba");

        // Result: wins first. Equal results keep the order newest first.
        CHECK(Order(games, { { SortColumn::RESULT, true } }) == "dacb");
        CHECK(Order(games, { { SortColumn::RESULT, false } }) == "bcda");

        // Color: White first. Ties: the newest first.
        CHECK(Order(games, { { SortColumn::COLOR, true } }) == "cadb");
        CHECK(Order(games, { { SortColumn::COLOR, false } }) == "dbca");

        // Opponent: by name, upper and lower case do not matter (Alice, bob, carl, Dora).
        CHECK(Order(games, { { SortColumn::OPPONENT, true } }) == "bacd");
        CHECK(Order(games, { { SortColumn::OPPONENT, false } }) == "dcab");

        // Time: 3+2 is 260 s, 10 min is 600 s, 15+10 is 1300 s, the daily game is a day.
        CHECK(Order(games, { { SortColumn::TIME, true } }) == "dabc");
        CHECK(Order(games, { { SortColumn::TIME, false } }) == "cbad");

        // Moves.
        CHECK(Order(games, { { SortColumn::MOVES, true } }) == "cdab");
        CHECK(Order(games, { { SortColumn::MOVES, false } }) == "badc");
    }

    void TestSortMissingValues()
    {
        using core::Color;
        using core::Outcome;
        using core::SortColumn;

        // Accuracy is the one of the user. Games without it are last in both directions.
        std::vector<core::GameSummary> games{
            Played("a", 100, Color::WHITE, Outcome::WIN, "x", "600", 1),
            Played("b", 200, Color::BLACK, Outcome::WIN, "x", "600", 1),
            Played("c", 300, Color::WHITE, Outcome::WIN, "x", "600", 1),
            Played("d", 400, Color::WHITE, Outcome::WIN, "x", "600", 1),
        };
        games[0].whiteAccuracy = 80.0;
        games[0].blackAccuracy = 20.0;
        games[1].whiteAccuracy = 10.0;
        games[1].blackAccuracy = 90.0;  // The user is Black: this one counts.
        games[2].whiteAccuracy = 50.0;

        CHECK(Order(games, { { SortColumn::ACCURACY, true } }) == "cabd");
        CHECK(Order(games, { { SortColumn::ACCURACY, false } }) == "bacd");

        // Opening: by name, and games without one are last in both directions.
        games[0].opening = "Sicilian Defense";
        games[1].opening = "french defense";
        games[2].opening = "";
        games[3].opening = "Caro-Kann";

        CHECK(Order(games, { { SortColumn::OPENING, true } }) == "dbac");
        CHECK(Order(games, { { SortColumn::OPENING, false } }) == "abdc");
    }

    void TestSortSeveralKeys()
    {
        using core::Color;
        using core::Outcome;
        using core::SortColumn;

        const std::vector<core::GameSummary> games{
            Played("a", 100, Color::WHITE, Outcome::WIN, "x", "600", 1),
            Played("b", 200, Color::BLACK, Outcome::WIN, "x", "600", 1),
            Played("c", 300, Color::WHITE, Outcome::LOSS, "x", "600", 1),
            Played("d", 400, Color::BLACK, Outcome::LOSS, "x", "600", 1),
        };

        // The first key decides, the second one breaks ties.
        CHECK(Order(games, { { SortColumn::COLOR, true }, { SortColumn::RESULT, true } }) == "acbd");
        CHECK(Order(games, { { SortColumn::RESULT, true }, { SortColumn::COLOR, false } }) == "badc");
        CHECK(Order(games, { { SortColumn::RESULT, true }, { SortColumn::DATE, true } }) == "abcd");

        // Only some of the games are sorted, and nothing else is touched.
        std::vector<std::size_t> some{ 3, 0, 2 };
        core::sort_games(games, some, { { SortColumn::DATE, true } });
        CHECK(some == (std::vector<std::size_t>{ 0, 2, 3 }));

        std::vector<std::size_t> none;
        core::sort_games(games, none, { { SortColumn::DATE, true } });
        CHECK(none.empty());
    }

    void TestEstimatedDuration()
    {
        CHECK(core::estimated_duration("600") == 600);
        CHECK(core::estimated_duration("180+2") == 260);
        CHECK(core::estimated_duration("900+10") == 1300);
        CHECK(core::estimated_duration("1/86400") == 86400);
        CHECK(core::estimated_duration("1/259200") == 259200);

        // What cannot be read is the longest, so such games come last when sorting from short to long.
        CHECK(core::estimated_duration("?") > core::estimated_duration("1/259200"));
        CHECK(core::estimated_duration("") > core::estimated_duration("1/259200"));
        CHECK(core::estimated_duration("1/x") > core::estimated_duration("1/259200"));
    }

} // namespace

int main()
{
    TestSort();
    TestAdd();
    TestFilter();
    TestTimeClasses();
    TestSortColumns();
    TestSortMissingValues();
    TestSortSeveralKeys();
    TestEstimatedDuration();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
