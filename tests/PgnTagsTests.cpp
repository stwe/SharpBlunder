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
#include "core/OpeningBook.h"
#include "core/Pgn.h"
#include "core/PgnTags.h"

namespace
{
    using sg::chess::core::Opening;
    using sg::chess::core::PgnHeaders;

    std::string Date(const char* t_text)
    {
        return sg::chess::core::normalize_date(t_text).value_or("<error>");
    }

    void TestDate()
    {
        CHECK(Date("2026.11.24") == "2026.11.24");
        CHECK(Date("2026-11-24") == "2026.11.24");
        CHECK(Date("2026/11/24") == "2026.11.24");
        CHECK(Date("24.11.2026") == "2026.11.24");
        CHECK(Date("2026.1.5") == "2026.01.05");
        CHECK(Date("5.1.2026") == "2026.01.05");
        CHECK(Date("  2026.11.24  ") == "2026.11.24");

        // PGN writes what is not known as question marks.
        CHECK(Date("????.??.??") == "????.??.??");
        CHECK(Date("2026.??.??") == "2026.??.??");
        CHECK(Date("2026.11.??") == "2026.11.??");
        CHECK(Date("2026.11.?") == "2026.11.??");
        CHECK(Date("????.11.24") == "????.11.24");

        // Days that do not exist.
        CHECK(Date("2026.02.29") == "<error>");
        CHECK(Date("2024.02.29") == "2024.02.29");
        CHECK(Date("????.02.29") == "????.02.29");
        CHECK(Date("????.02.30") == "<error>");
        CHECK(Date("2026.04.31") == "<error>");
        CHECK(Date("2026.13.01") == "<error>");
        CHECK(Date("2026.00.10") == "<error>");
        CHECK(Date("2026.10.00") == "<error>");
        CHECK(Date("2026.??.24") == "<error>");

        // Not dates.
        CHECK(Date("") == "<error>");
        CHECK(Date("yesterday") == "<error>");
        CHECK(Date("2026.11") == "<error>");
        CHECK(Date("26.11.24") == "<error>");
        CHECK(Date("2026.11.24.1") == "<error>");
    }

    void TestEco()
    {
        CHECK(sg::chess::core::is_eco_code("A00"));
        CHECK(sg::chess::core::is_eco_code("B90"));
        CHECK(sg::chess::core::is_eco_code("E99"));
        CHECK(!sg::chess::core::is_eco_code("F00"));
        CHECK(!sg::chess::core::is_eco_code("B9"));
        CHECK(!sg::chess::core::is_eco_code("b90"));
        CHECK(!sg::chess::core::is_eco_code("B900"));
        CHECK(!sg::chess::core::is_eco_code(""));
    }

    void TestOpeningTags()
    {
        PgnHeaders headers{ PgnHeaders::MakeDefault("2026.11.24") };

        sg::chess::core::set_opening_tags(headers,
                                          Opening{ "B90", "Sicilian Defense: Najdorf Variation, English Attack" });
        CHECK(headers.Get("ECO") == "B90");
        CHECK(headers.Get("Opening") == "Sicilian Defense");
        CHECK(headers.Get("Variation") == "Najdorf Variation");
        CHECK(headers.Get("SubVariation") == "English Attack");

        // Old values do not stay.
        sg::chess::core::set_opening_tags(headers, Opening{ "C60", "Ruy Lopez" });
        CHECK(headers.Get("ECO") == "C60");
        CHECK(headers.Get("Opening") == "Ruy Lopez");
        CHECK(headers.Get("Variation").empty());
        CHECK(headers.Get("SubVariation").empty());

        sg::chess::core::set_opening_tags(headers, Opening{ "C65", "Ruy Lopez: Berlin Defense" });
        CHECK(headers.Get("Opening") == "Ruy Lopez");
        CHECK(headers.Get("Variation") == "Berlin Defense");

        // A broken ECO code is left out.
        sg::chess::core::set_opening_tags(headers, Opening{ "??", "Ruy Lopez" });
        CHECK(headers.Get("ECO").empty());

        sg::chess::core::clear_opening_tags(headers);
        CHECK(headers.Get("Opening").empty());
        CHECK(headers.Get("Variation").empty());
        CHECK(headers.Get("Event") == "Analysis");
    }
} // namespace

int main()
{
    TestDate();
    TestEco();
    TestOpeningTags();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
