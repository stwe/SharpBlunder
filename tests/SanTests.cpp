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
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>
#include "Check.h"
#include "core/Position.h"
#include "core/PositionInfo.h"
#include "core/San.h"

namespace
{
    using sg::chess::core::Move;
    using sg::chess::core::PositionInfo;

    // A position with some of its legal moves. from_san() only looks at the moves it is given.
    PositionInfo Info(const char* t_fen, const std::initializer_list<const char*> t_moves)
    {
        PositionInfo info;
        info.position = sg::chess::core::Position::FromFen(t_fen);

        for (const char* uci : t_moves)
        {
            info.legalMoves.push_back(*Move::FromUci(uci));
        }

        return info;
    }

    std::string Uci(const PositionInfo& t_info, const char* t_san)
    {
        const auto move{ sg::chess::core::from_san(t_info, t_san) };

        return move ? move->ToUci() : "none";
    }

    void TestStartPosition()
    {
        const PositionInfo info{ Info(sg::chess::core::Position::START_FEN.data(),
                                      { "e2e4", "e2e3", "d2d4", "g1f3", "g1h3", "b1c3", "b1a3", "a2a3" }) };

        CHECK(Uci(info, "e4") == "e2e4");
        CHECK(Uci(info, "Nf3") == "g1f3");
        CHECK(Uci(info, "Nc3") == "b1c3");
        CHECK(Uci(info, "a3") == "a2a3");

        // Check, checkmate and glyphs are ignored.
        CHECK(Uci(info, "Nf3+") == "g1f3");
        CHECK(Uci(info, "e4!?") == "e2e4");
        CHECK(Uci(info, "d4#") == "d2d4");

        // Not a legal move here, or not a move at all.
        CHECK(Uci(info, "e5") == "none");
        CHECK(Uci(info, "Nf6") == "none");
        CHECK(Uci(info, "Ke9") == "none");
        CHECK(Uci(info, "Xf3") == "none");
        CHECK(Uci(info, "") == "none");
        CHECK(Uci(info, "e") == "none");
    }

    void TestCastling()
    {
        const PositionInfo info{ Info("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", { "e1g1", "e1c1", "e1d1", "e1f1" }) };

        CHECK(Uci(info, "O-O") == "e1g1");
        CHECK(Uci(info, "O-O-O") == "e1c1");
        CHECK(Uci(info, "0-0") == "e1g1");
        CHECK(Uci(info, "0-0-0") == "e1c1");
        CHECK(Uci(info, "O-O+") == "e1g1");
        CHECK(Uci(info, "O-O-") == "none");

        // The king's step of one square is no castling.
        const PositionInfo noCastling{ Info("r3k2r/8/8/8/8/8/8/R3K2R w - - 0 1", { "e1d1", "e1f1" }) };
        CHECK(Uci(noCastling, "O-O") == "none");
    }

    void TestDisambiguation()
    {
        const PositionInfo files{ Info("k7/8/8/8/8/8/8/R6R w - - 0 1", { "a1d1", "h1d1", "a1a2", "h1h2" }) };

        CHECK(Uci(files, "Rad1") == "a1d1");
        CHECK(Uci(files, "Rhd1") == "h1d1");

        // Both rooks can go to d1: without a hint the text is ambiguous.
        CHECK(Uci(files, "Rd1") == "none");

        const PositionInfo ranks{ Info("k7/8/8/8/8/R7/8/R3K3 w - - 0 1", { "a1a2", "a3a2", "e1e2" }) };

        CHECK(Uci(ranks, "R1a2") == "a1a2");
        CHECK(Uci(ranks, "R3a2") == "a3a2");
        CHECK(Uci(ranks, "Ra2") == "none");
        CHECK(Uci(ranks, "Ra1a2") == "a1a2");
    }

    void TestPawns()
    {
        const PositionInfo info{ Info("k7/4P3/8/3p4/2P1P3/8/8/K7 w - - 0 1",
                                      { "c4d5", "e4d5", "c4c5", "e4e5", "e7e8q", "e7e8r", "e7e8b", "e7e8n" }) };

        CHECK(Uci(info, "exd5") == "e4d5");
        CHECK(Uci(info, "cxd5") == "c4d5");
        CHECK(Uci(info, "e5") == "e4e5");

        // Without the file of the pawn, the capture is ambiguous.
        CHECK(Uci(info, "xd5") == "none");

        CHECK(Uci(info, "e8=Q") == "e7e8q");
        CHECK(Uci(info, "e8=N") == "e7e8n");
        CHECK(Uci(info, "e8R") == "e7e8r");
        CHECK(Uci(info, "e8=Q+") == "e7e8q");

        // A promotion needs the piece.
        CHECK(Uci(info, "e8") == "none");
    }
}

int main()
{
    TestStartPosition();
    TestCastling();
    TestDisambiguation();
    TestPawns();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
