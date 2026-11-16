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
#include <stdexcept>
#include <string>
#include "Check.h"
#include "core/OpeningBook.h"
#include "core/Position.h"

namespace
{
    // Opening names and positions as in assets/openings/openings.tsv.
    constexpr const char* SAMPLE{
        "eco\tname\tepd\n"
        "B00\tKing's Pawn Game\trnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq -\n"
        "\n"
        "C20\tKing's Pawn Game: Test\trnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -\n"
        "C21\tSecond Entry\trnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -\n"
    };

    sg::chess::core::Position FromFen(const char* t_fen)
    {
        return sg::chess::core::Position::FromFen(t_fen);
    }

    void TestFind()
    {
        const auto book{ sg::chess::core::OpeningBook::FromTsv(SAMPLE) };

        CHECK(book.Size() == 2);

        // The en passant square and the counters of the FEN are not part of the comparison.
        const auto afterE4{ book.Find(FromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1")) };
        CHECK(afterE4.has_value());
        CHECK(afterE4 && afterE4->eco == "B00");
        CHECK(afterE4 && afterE4->name == "King's Pawn Game");

        const auto sameWithoutEp{ book.Find(FromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 12 40")) };
        CHECK(sameWithoutEp.has_value());
    }

    void TestSideAndCastlingCount()
    {
        const auto book{ sg::chess::core::OpeningBook::FromTsv(SAMPLE) };

        // Same pieces, other side to move or other castling rights: another position.
        CHECK(!book.Find(FromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1")));
        CHECK(!book.Find(FromFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b Kkq - 0 1")));
        CHECK(!book.Find(sg::chess::core::Position::FromFen(sg::chess::core::Position::START_FEN)));
    }

    void TestFirstEntryWins()
    {
        const auto book{ sg::chess::core::OpeningBook::FromTsv(SAMPLE) };
        const auto found{ book.Find(FromFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2")) };

        CHECK(found && found->eco == "C20");
    }

    void TestMalformedLine()
    {
        bool thrown{ false };

        try
        {
            static_cast<void>(sg::chess::core::OpeningBook::FromTsv("eco\tname\tepd\nB00\tonly two columns\n"));
        }
        catch (const std::runtime_error&)
        {
            thrown = true;
        }

        CHECK(thrown);
    }

    // The real data: names of well known openings, found by the FEN of the position after the moves.
    void TestRealData()
    {
        const auto book{ sg::chess::core::OpeningBook::FromFile(std::string{ SHARPBLUNDER_ASSET_DIR } +
                                                               "/openings/openings.tsv") };

        CHECK(book.Size() > 3000);

        // 1.e4 c5 2.Nf3 d6 3.d4 cxd4 4.Nxd4 Nf6 5.Nc3 a6
        const auto najdorf{ book.Find(FromFen("rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6")) };
        CHECK(najdorf && najdorf->eco == "B90");
        CHECK(najdorf && najdorf->name == "Sicilian Defense: Najdorf Variation");

        // 1.e4 e5 2.Nf3 Nc6 3.Bb5 a6 4.Ba4 Nf6 5.O-O
        const auto ruyLopez{ book.Find(FromFen("r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 3 5")) };
        CHECK(ruyLopez && ruyLopez->name == "Ruy Lopez: Morphy Defense");

        // The same position by 1.d4 d5 2.c4 e6 3.Nc3 Nf6 and by 1.c4 e6 2.Nc3 d5 3.d4 Nf6: one name.
        const auto qgd{ book.Find(FromFen("rnbqkb1r/ppp2ppp/4pn2/3p4/2PP4/2N5/PP2PPPP/R1BQKBNR w KQkq - 2 4")) };
        CHECK(qgd && qgd->name == "Queen's Gambit Declined: Normal Defense");
    }
}

int main()
{
    TestFind();
    TestSideAndCastlingCount();
    TestFirstEntryWins();
    TestMalformedLine();
    TestRealData();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
