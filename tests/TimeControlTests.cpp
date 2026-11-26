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
#include "Check.h"
#include "core/TimeControl.h"

namespace
{
    using sg::chess::core::TimeControl;

    // What a person types, and what ends up in the PGN file.
    std::string Pgn(const char* t_input)
    {
        return sg::chess::core::time_control_from_input(t_input).value_or("<error>");
    }

    void TestInput()
    {
        // Without units: minutes at the start, seconds per move.
        CHECK(Pgn("15+10") == "900+10");
        CHECK(Pgn("10") == "600");
        CHECK(Pgn("5|3") == "300+3");
        CHECK(Pgn(" 3 + 2 ") == "180+2");
        CHECK(Pgn("1+0") == "60");

        // With units.
        CHECK(Pgn("15 min + 10 s") == "900+10");
        CHECK(Pgn("15min+10sec") == "900+10");
        CHECK(Pgn("90 sec") == "90");
        CHECK(Pgn("90s") == "90");
        CHECK(Pgn("600s + 5") == "600+5");
        CHECK(Pgn("1h") == "3600");
        CHECK(Pgn("1h+30") == "3600+30");
        CHECK(Pgn("1 h 30 min + 30 s") == "5400+30");
        CHECK(Pgn("1h 30min") == "5400");
        CHECK(Pgn("15 MIN + 10 S") == "900+10");
        CHECK(Pgn("2 minutes + 1 second") == "120+1");

        // Decimals, with a point or a comma.
        CHECK(Pgn("1,5") == "90");
        CHECK(Pgn("0.5+0") == "30");
        CHECK(Pgn("0,5 min + 2") == "30+2");

        // Nothing to play with the increment alone is fine, no time at all is not.
        CHECK(Pgn("0+5") == "0+5");
        CHECK(Pgn("0") == "<error>");
        CHECK(Pgn("0+0") == "<error>");
    }

    void TestBadInput()
    {
        CHECK(Pgn("") == "<error>");
        CHECK(Pgn("   ") == "<error>");
        CHECK(Pgn("abc") == "<error>");
        CHECK(Pgn("15+") == "<error>");
        CHECK(Pgn("+10") == "<error>");
        CHECK(Pgn("15 parsecs") == "<error>");
        CHECK(Pgn("15+10+5") == "<error>");
        CHECK(Pgn("15 10") == "<error>");   // only the first amount may go without a unit
        CHECK(Pgn("1h30") == "<error>");
        CHECK(Pgn("1.2.3") == "<error>");
        CHECK(Pgn("99999999") == "<error>");
        CHECK(Pgn("-5") == "<error>");
    }

    void TestPgnValues()
    {
        // A correct tag value that is not a simple time control is kept as it is.
        CHECK(Pgn("40/7200:3600") == "40/7200:3600");
        CHECK(Pgn("40/7200:3600+30") == "40/7200:3600+30");
        CHECK(Pgn("40/7200:20/2400:600+5") == "40/7200:20/2400:600+5");
        CHECK(Pgn("*120") == "*120");
        CHECK(Pgn("?") == "?");
        CHECK(Pgn("-") == "-");

        CHECK(sg::chess::core::is_pgn_time_control("300"));
        CHECK(sg::chess::core::is_pgn_time_control("300+5"));
        CHECK(sg::chess::core::is_pgn_time_control("40/7200:3600"));
        CHECK(!sg::chess::core::is_pgn_time_control(""));
        CHECK(!sg::chess::core::is_pgn_time_control("300+"));
        CHECK(!sg::chess::core::is_pgn_time_control("300+5:60"));  // an increment is only allowed in the last period
        CHECK(!sg::chess::core::is_pgn_time_control("*60:300"));   // so is an hourglass
        CHECK(!sg::chess::core::is_pgn_time_control("40/"));
        CHECK(!sg::chess::core::is_pgn_time_control("15 min"));
        CHECK(!sg::chess::core::is_pgn_time_control("300:"));
    }

    void TestRoundTrip()
    {
        const TimeControl rapid{ 900, 10 };
        CHECK(sg::chess::core::time_control_to_pgn(rapid) == "900+10");
        CHECK(sg::chess::core::time_control_to_pgn({ 300, 0 }) == "300");
        CHECK(sg::chess::core::time_control_from_pgn("900+10") == rapid);
        CHECK(sg::chess::core::time_control_from_pgn("300") == TimeControl(300, 0));
        CHECK(!sg::chess::core::time_control_from_pgn("40/7200:3600"));
        CHECK(!sg::chess::core::time_control_from_pgn("?"));
        CHECK(!sg::chess::core::time_control_from_pgn("-"));
        CHECK(!sg::chess::core::time_control_from_pgn("*60"));

        CHECK(sg::chess::core::describe_time_control({ 900, 10 }) == "15 min + 10 s");
        CHECK(sg::chess::core::describe_time_control({ 90, 0 }) == "90 s");
        CHECK(sg::chess::core::describe_time_control({ 5400, 30 }) == "1 h 30 min + 30 s");
        CHECK(sg::chess::core::describe_time_control({ 3690, 0 }) == "1 h 1 min 30 s");
        CHECK(sg::chess::core::describe_time_control({ 0, 5 }) == "0 s + 5 s");

        // What the field shows reads back to the same tag value.
        for (const char* value : { "900+10", "300", "60", "180+2", "5400+30", "3690", "90", "0+5", "86400",
                                   "40/7200:3600", "*120", "?", "-" })
        {
            const std::string shown{ sg::chess::core::time_control_for_input(value) };
            CHECK(sg::chess::core::time_control_from_input(shown) == std::optional<std::string>{ value });
        }

        CHECK(sg::chess::core::time_control_for_input("900+10") == "15 min + 10 s");
        CHECK(sg::chess::core::time_control_for_input("40/7200:3600") == "40/7200:3600");
    }
} // namespace

int main()
{
    TestInput();
    TestBadInput();
    TestPgnValues();
    TestRoundTrip();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
