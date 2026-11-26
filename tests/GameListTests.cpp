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
} // namespace

int main()
{
    TestSort();
    TestAdd();
    TestFilter();
    TestTimeClasses();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
