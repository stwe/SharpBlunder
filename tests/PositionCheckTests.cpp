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
#include "core/Position.h"
#include "core/PositionCheck.h"

namespace
{
    // The text of the problem, or an empty text if the position is fine.
    std::string Problem(const char* t_fen)
    {
        const auto problem{ sg::chess::core::check_position(sg::chess::core::Position::FromFen(t_fen)) };

        return problem ? *problem : "";
    }

    bool Refused(const char* t_fen)
    {
        return !Problem(t_fen).empty();
    }

    void TestValid()
    {
        CHECK(Problem("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").empty());
        CHECK(Problem("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1").empty());

        // The side to move may be in check.
        CHECK(Problem("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1").empty());

        // A king next to a pawn that cannot attack it (a White pawn attacks upward only).
        CHECK(Problem("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1").empty());
    }

    void TestKings()
    {
        CHECK(Problem("8/8/8/8/8/8/8/4K3 w - - 0 1").find("one king") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/8/8/8/8 w - - 0 1").find("White needs exactly one king") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/8/8/8/3KK3 w - - 0 1").find("White needs exactly one king") != std::string::npos);
        CHECK(Problem("3kk3/8/8/8/8/8/8/4K3 w - - 0 1").find("Black needs exactly one king") != std::string::npos);
    }

    void TestPawnsAndPieces()
    {
        CHECK(Problem("P3k3/8/8/8/8/8/8/4K3 w - - 0 1").find("first or the last rank") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/8/8/8/p3K3 w - - 0 1").find("first or the last rank") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/PPPPPPPP/P7/8/4K3 w - - 0 1").find("too many") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/8/QQQQQQQQ/QQQQQQQQ/QQQK4 w - - 0 1").find("too many") != std::string::npos);
    }

    void TestSideNotToMoveInCheck()
    {
        // The rook gives check to the Black king, but White is to move: that cannot have happened.
        CHECK(Problem("4k3/8/8/8/8/8/4R3/4K3 w - - 0 1").find("Black is in check") != std::string::npos);
        CHECK(Problem("4k3/8/8/8/8/8/4R3/4K3 b - - 0 1").empty());

        // Every kind of attacker gives check to the Black king on e8.
        CHECK(Refused("4k3/8/3N4/8/8/8/8/4K3 w - - 0 1"));  // knight
        CHECK(Refused("4k3/8/5N2/8/8/8/8/4K3 w - - 0 1"));  // knight
        CHECK(Refused("4k3/3P4/8/8/8/8/8/4K3 w - - 0 1"));  // pawn
        CHECK(Refused("4k3/8/8/8/B7/8/8/4K3 w - - 0 1"));   // bishop
        CHECK(Refused("4k3/8/8/8/Q7/8/8/4K3 w - - 0 1"));   // queen on a diagonal
        CHECK(Refused("4k3/8/8/8/4Q3/8/8/4K3 w - - 0 1"));  // queen on a file

        // These pieces do not give check.
        CHECK(!Refused("4k3/8/8/8/1B6/8/8/4K3 w - - 0 1"));  // bishop on the wrong diagonal
        CHECK(!Refused("4k3/4P3/8/8/8/8/8/4K3 w - - 0 1"));  // a pawn in front of the king attacks nothing there
        CHECK(!Refused("8/8/8/8/8/8/8/k1K5 w - - 0 1"));     // kings two squares apart

        // Kings next to each other.
        CHECK(Refused("8/8/8/8/8/8/k7/1K6 w - - 0 1"));

        // A piece in between blocks the line.
        CHECK(Problem("4k3/8/8/8/8/4n3/4R3/4K3 w - - 0 1").empty());
    }
}

int main()
{
    TestValid();
    TestKings();
    TestPawnsAndPieces();
    TestSideNotToMoveInCheck();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
