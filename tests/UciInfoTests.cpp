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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include "Check.h"
#include "core/MoveQuality.h"
#include "core/Score.h"
#include "engine/UciInfo.h"

namespace
{
    void TestCentipawnLine()
    {
        const auto line{ sg::chess::engine::parse_info_line(
            "info depth 18 seldepth 24 multipv 2 score cp -31 nodes 913450 nps 1000000 hashfull 12 "
            "tbhits 0 time 913 pv e7e5 g1f3 b8c6") };

        CHECK(line.has_value());
        if (!line)
        {
            return;
        }

        CHECK(line->depth == 18);
        CHECK(line->multipv == 2);
        CHECK(line->score.kind == sg::chess::core::Score::Kind::CENTIPAWNS);
        CHECK(line->score.value == -31);
        CHECK(line->pv.size() == 3);
        CHECK(line->pv[0].ToUci() == "e7e5");
        CHECK(line->pv[2].ToUci() == "b8c6");
    }

    void TestMissingMultiPvMeansFirstLine()
    {
        const auto line{ sg::chess::engine::parse_info_line("info depth 1 score cp 20 pv e2e4") };

        CHECK(line.has_value());
        CHECK(line && line->multipv == 1);
    }

    void TestMateAndPromotion()
    {
        const auto line{ sg::chess::engine::parse_info_line("info depth 9 score mate -3 pv a7a8q h1h2") };

        CHECK(line.has_value());
        if (!line)
        {
            return;
        }

        CHECK(line->score.kind == sg::chess::core::Score::Kind::MATE);
        CHECK(line->score.value == -3);
        CHECK(line->pv[0].promotion.has_value());
    }

    void TestSkippedLines()
    {
        using sg::chess::engine::parse_info_line;

        CHECK(!parse_info_line("info string NNUE evaluation using nn-1111cefa1111.nnue"));
        CHECK(!parse_info_line("info depth 5 currmove e2e4 currmovenumber 1"));
        CHECK(!parse_info_line("info depth 0 score mate 0"));
        CHECK(!parse_info_line("info depth 12 score cp 40 lowerbound pv e2e4"));
        CHECK(!parse_info_line("info depth 12 score cp 40 pv e2e9"));
        CHECK(!parse_info_line("bestmove e2e4 ponder e7e5"));
        CHECK(!parse_info_line(""));
    }

    void TestScoreSides()
    {
        using sg::chess::core::Score;

        const Score blackToMoveIsWinning{ Score::Kind::CENTIPAWNS, 150 };
        CHECK(blackToMoveIsWinning.FromSideToMove(false).value == -150);
        CHECK(blackToMoveIsWinning.FromSideToMove(true).value == 150);
    }

    void TestScoreText()
    {
        using sg::chess::core::Score;

        CHECK((Score{ Score::Kind::CENTIPAWNS, 35 }.ToString() == "+0.35"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, -120 }.ToString() == "-1.20"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, 0 }.ToString() == "+0.00"));
        CHECK((Score{ Score::Kind::MATE, 3 }.ToString() == "M3"));
        CHECK((Score{ Score::Kind::MATE, -2 }.ToString() == "-M2"));
    }

    void TestCentipawnsAndQuality()
    {
        using sg::chess::core::MoveQuality;
        using sg::chess::core::Score;

        CHECK((Score{ Score::Kind::CENTIPAWNS, -35 }.Centipawns() == -35));

        // A mate is worth more than any advantage, a shorter mate more than a longer one, and the sign stays.
        CHECK((Score{ Score::Kind::MATE, 5 }.Centipawns() > 5000));
        CHECK((Score{ Score::Kind::MATE, 2 }.Centipawns() > Score{ Score::Kind::MATE, 5 }.Centipawns()));
        CHECK((Score{ Score::Kind::MATE, -2 }.Centipawns() < -5000));

        CHECK(sg::chess::core::classify_loss(-3) == MoveQuality::GOOD);
        CHECK(sg::chess::core::classify_loss(50) == MoveQuality::GOOD);
        CHECK(sg::chess::core::classify_loss(51) == MoveQuality::OKAY);
        CHECK(sg::chess::core::classify_loss(150) == MoveQuality::OKAY);
        CHECK(sg::chess::core::classify_loss(151) == MoveQuality::WEAK);

        // Missing a mate in 3 for a normal advantage is a weak move.
        const int lost{ Score{ Score::Kind::MATE, 3 }.Centipawns() - Score{ Score::Kind::CENTIPAWNS, 400 }.Centipawns() };
        CHECK(sg::chess::core::classify_loss(lost) == MoveQuality::WEAK);
    }

    void TestDescribe()
    {
        using sg::chess::core::Score;

        CHECK((Score{ Score::Kind::CENTIPAWNS, 0 }.Describe() == "Equal"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, -25 }.Describe() == "Equal"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, 40 }.Describe() == "White is slightly better"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, -120 }.Describe() == "Black is better"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, 350 }.Describe() == "White is much better"));
        CHECK((Score{ Score::Kind::CENTIPAWNS, -900 }.Describe() == "Black is winning"));
        CHECK((Score{ Score::Kind::MATE, 3 }.Describe() == "White mates in 3"));
        CHECK((Score{ Score::Kind::MATE, -2 }.Describe() == "Black mates in 2"));
    }

    void TestWhiteShare()
    {
        using sg::chess::core::Score;

        CHECK((Score{ Score::Kind::CENTIPAWNS, 0 }.WhiteShare() == 0.5));
        CHECK((Score{ Score::Kind::MATE, 4 }.WhiteShare() == 1.0));
        CHECK((Score{ Score::Kind::MATE, -4 }.WhiteShare() == 0.0));

        const double small{ Score{ Score::Kind::CENTIPAWNS, 100 }.WhiteShare() };
        const double big{ Score{ Score::Kind::CENTIPAWNS, 1000 }.WhiteShare() };
        const double mirrored{ Score{ Score::Kind::CENTIPAWNS, -100 }.WhiteShare() };

        // A smooth curve: it rises, it stays below 1, and it is symmetric.
        CHECK(small > 0.5 && small < big && big < 1.0);
        CHECK(std::abs(small + mirrored - 1.0) < 1e-9);
    }
} // namespace

int main()
{
    TestCentipawnLine();
    TestMissingMultiPvMeansFirstLine();
    TestMateAndPromotion();
    TestSkippedLines();
    TestScoreSides();
    TestScoreText();
    TestCentipawnsAndQuality();
    TestDescribe();
    TestWhiteShare();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
