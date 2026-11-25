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

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include "Check.h"
#include "core/DrawRules.h"
#include "core/GameState.h"
#include "core/Move.h"
#include "core/Position.h"
#include "core/PositionInfo.h"
#include "core/RulesProvider.h"
#include "core/Square.h"
#include "core/Util.h"

namespace
{
    using sg::chess::core::Color;
    using sg::chess::core::DrawReason;
    using sg::chess::core::GameState;
    using sg::chess::core::Move;
    using sg::chess::core::Position;
    using sg::chess::core::PositionInfo;

    // The moves that the tests play. A move is legal if a piece of the side to move stands on its first square and
    // no piece of that side stands on its second. That is all these tests need to know about the rules.
    constexpr std::array<const char*, 10> MOVES{ "g1f3", "g8f6", "f3g1", "f6g8", "e2e4", "e7e5",
                                                 "b1c3", "c3b1", "b8c6", "c6b8" };

    // Rules for the moves above, without Stockfish: they move a piece and count the halfmoves.
    class ShuffleRules final : public sg::chess::core::RulesProvider
    {
    public:
        [[nodiscard]] PositionInfo Describe(const Position& t_position) override
        {
            PositionInfo info;
            info.position = t_position;

            for (const char* uci : MOVES)
            {
                const Move move{ *Move::FromUci(uci) };
                const auto piece{ t_position.At(move.from) };
                const auto target{ t_position.At(move.to) };

                if (piece && piece->color == t_position.SideToMove() && !(target && target->color == piece->color))
                {
                    info.legalMoves.push_back(move);
                }
            }

            return info;
        }

        [[nodiscard]] PositionInfo Play(const Position& t_position, const Move& t_move) override
        {
            // The pieces as 64 characters, rank 1 first.
            std::array<char, 64> board{};
            board.fill(' ');

            const std::string fen{ t_position.ToFen() };
            int file{ 0 };
            int rank{ 7 };
            std::size_t pos{ 0 };

            for (; fen[pos] != ' '; ++pos)
            {
                if (fen[pos] == '/')
                {
                    --rank;
                    file = 0;
                }
                else if (fen[pos] >= '1' && fen[pos] <= '8')
                {
                    file += fen[pos] - '0';
                }
                else
                {
                    board[static_cast<std::size_t>(rank * 8 + file)] = fen[pos];
                    ++file;
                }
            }

            const auto from{ static_cast<std::size_t>(t_move.from) };
            const auto to{ static_cast<std::size_t>(t_move.to) };
            const bool irreversible{ board[from] == 'P' || board[from] == 'p' || board[to] != ' ' };

            board[to] = board[from];
            board[from] = ' ';

            std::string placement;
            for (int r{ 7 }; r >= 0; --r)
            {
                int empty{ 0 };

                for (int f{ 0 }; f < 8; ++f)
                {
                    const char c{ board[static_cast<std::size_t>(r * 8 + f)] };

                    if (c == ' ')
                    {
                        ++empty;
                        continue;
                    }

                    if (empty > 0)
                    {
                        placement += std::to_string(empty);
                        empty = 0;
                    }

                    placement += c;
                }

                if (empty > 0)
                {
                    placement += std::to_string(empty);
                }

                if (r > 0)
                {
                    placement += '/';
                }
            }

            const bool white{ t_position.SideToMove() == Color::WHITE };
            const int halfmoves{ irreversible ? 0 : t_position.HalfmoveClock() + 1 };
            const int fullmoves{ t_position.FullmoveNumber() + (white ? 0 : 1) };

            const std::string side{ white ? " b " : " w " };

            return Describe(Position::FromFen(placement + side + "KQkq - " + std::to_string(halfmoves) + " "
                                              + std::to_string(fullmoves)));
        }
    };

    void Shuffle(GameState& t_game, const int t_cycles)
    {
        for (int cycle{ 0 }; cycle < t_cycles; ++cycle)
        {
            for (const char* uci : { "g1f3", "g8f6", "f3g1", "f6g8" })
            {
                CHECK(t_game.Play(*Move::FromUci(uci)));
            }
        }
    }

    PositionInfo Info(const char* t_fen)
    {
        PositionInfo info;
        info.position = Position::FromFen(t_fen);
        info.legalMoves.push_back(*Move::FromUci("e2e4"));  // the game goes on: any legal move will do

        return info;
    }

    void TestInsufficientMaterial()
    {
        using sg::chess::core::is_insufficient_material;

        const auto dead = [](const char* t_fen) { return is_insufficient_material(Position::FromFen(t_fen)); };

        CHECK(dead("8/8/8/4k3/8/3K4/8/8 w - - 0 1"));              // king against king
        CHECK(dead("8/8/8/4k3/8/3K4/8/5B2 w - - 0 1"));            // a bishop
        CHECK(dead("8/8/8/4k3/8/3K4/8/6N1 w - - 0 1"));            // a knight
        CHECK(dead("8/8/8/4k3/8/3K4/8/6n1 w - - 0 1"));            // the knight of the other side
        CHECK(dead("5b2/8/8/4k3/8/3K4/8/2B5 w - - 0 1"));          // bishops on the same color (c1, f8)
        CHECK(dead("8/8/8/4k3/8/3KB3/8/2B5 w - - 0 1"));           // two bishops on the same color (c1, e3)

        CHECK(!dead("2b5/8/8/4k3/8/3K4/8/2B5 w - - 0 1"));         // bishops on different colors (c1, c8)
        CHECK(!dead("8/8/8/4k3/8/3K4/8/5BN1 w - - 0 1"));          // a bishop and a knight
        CHECK(!dead("8/8/8/4k3/8/3K4/8/4NN2 w - - 0 1"));          // two knights
        CHECK(!dead("6n1/8/8/4k3/8/3K4/8/6N1 w - - 0 1"));         // a knight each
        CHECK(!dead("8/8/8/4k3/8/3K4/4P3/8 w - - 0 1"));           // a pawn
        CHECK(!dead("8/8/8/4k3/8/3K4/8/R7 w - - 0 1"));            // a rook
        CHECK(!dead("8/8/8/4k3/8/3K4/8/Q7 w - - 0 1"));            // a queen
        CHECK(!dead("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    }

    void TestRepetition()
    {
        using sg::chess::core::repetition_count;

        ShuffleRules rules;
        GameState game{ rules };

        CHECK(repetition_count(game, 0) == 1);

        // 1. Nf3 Nf6 2. Ng1 Ng8: the start position for the second time.
        Shuffle(game, 1);
        CHECK(repetition_count(game, game.CurrentId()) == 2);
        CHECK(!sg::chess::core::find_draw(game.Current().info, 2));

        // The position after 1. Nf3 has occurred once so far.
        CHECK(repetition_count(game, 1) == 1);

        Shuffle(game, 1);
        CHECK(repetition_count(game, game.CurrentId()) == 3);
        CHECK(sg::chess::core::find_draw(game.Current().info, 3) == DrawReason::THREEFOLD_REPETITION);

        // The position after 1. Nf3 is the same at the plies 1, 5 and 9.
        CHECK(repetition_count(game, 5) == 2);
        Shuffle(game, 1);
        CHECK(repetition_count(game, 9) == 3);

        Shuffle(game, 1);
        CHECK(repetition_count(game, game.CurrentId()) == 5);
        CHECK(sg::chess::core::find_draw(game.Current().info, 5) == DrawReason::FIVEFOLD_REPETITION);
    }

    void TestRepetitionAfterPawnMove()
    {
        using sg::chess::core::repetition_count;

        ShuffleRules rules;
        GameState game{ rules };

        // The start position twice, then 1. e4 e5. Nothing before the pawn moves can come back.
        Shuffle(game, 1);
        CHECK(game.Play(*Move::FromUci("e2e4")));
        CHECK(game.Play(*Move::FromUci("e7e5")));
        CHECK(repetition_count(game, game.CurrentId()) == 1);

        Shuffle(game, 1);
        CHECK(repetition_count(game, game.CurrentId()) == 2);
    }

    void TestRepetitionInVariation()
    {
        using sg::chess::core::repetition_count;

        ShuffleRules rules;
        GameState game{ rules };

        // Only the line that leads to a position counts, not the moves of other lines.
        Shuffle(game, 1);
        game.GoToStart();
        CHECK(game.Play(*Move::FromUci("b1c3")));
        CHECK(game.Play(*Move::FromUci("b8c6")));
        CHECK(game.Play(*Move::FromUci("c3b1")));
        CHECK(game.Play(*Move::FromUci("c6b8")));
        CHECK(repetition_count(game, game.CurrentId()) == 2);
    }

    void TestFindDraw()
    {
        using sg::chess::core::find_draw;

        // Rooks, so that there is enough material.
        constexpr const char* ROOKS{ "r3k3/8/8/8/8/8/8/R3K3 w - - " };
        const auto rooks = [&](const int t_halfmoves)
        { return Info((std::string{ ROOKS } + std::to_string(t_halfmoves) + " 80").c_str()); };

        CHECK(!find_draw(rooks(0), 1));
        CHECK(!find_draw(rooks(99), 2));
        CHECK(find_draw(rooks(100), 1) == DrawReason::FIFTY_MOVES);
        CHECK(find_draw(rooks(149), 1) == DrawReason::FIFTY_MOVES);
        CHECK(find_draw(rooks(150), 1) == DrawReason::SEVENTY_FIVE_MOVES);
        CHECK(find_draw(rooks(0), 3) == DrawReason::THREEFOLD_REPETITION);
        CHECK(find_draw(rooks(0), 4) == DrawReason::THREEFOLD_REPETITION);
        CHECK(find_draw(rooks(0), 5) == DrawReason::FIVEFOLD_REPETITION);

        // What ends the game at once comes before what only allows a claim.
        CHECK(find_draw(rooks(100), 3) == DrawReason::THREEFOLD_REPETITION);
        CHECK(find_draw(rooks(100), 5) == DrawReason::FIVEFOLD_REPETITION);
        CHECK(find_draw(rooks(150), 3) == DrawReason::SEVENTY_FIVE_MOVES);
        CHECK(find_draw(Info("8/8/8/4k3/8/3K4/8/8 w - - 100 80"), 3) == DrawReason::INSUFFICIENT_MATERIAL);

        // Checkmate and stalemate are not draws of this kind.
        PositionInfo mate{ rooks(150) };
        mate.legalMoves.clear();
        mate.inCheck = true;
        CHECK(!find_draw(mate, 5));

        PositionInfo stalemate{ rooks(150) };
        stalemate.legalMoves.clear();
        CHECK(!find_draw(stalemate, 5));
    }

    void TestTexts()
    {
        using sg::chess::core::describe_draw;
        using sg::chess::core::is_automatic_draw;

        CHECK(is_automatic_draw(DrawReason::INSUFFICIENT_MATERIAL));
        CHECK(is_automatic_draw(DrawReason::FIVEFOLD_REPETITION));
        CHECK(is_automatic_draw(DrawReason::SEVENTY_FIVE_MOVES));
        CHECK(!is_automatic_draw(DrawReason::THREEFOLD_REPETITION));
        CHECK(!is_automatic_draw(DrawReason::FIFTY_MOVES));

        CHECK(describe_draw(DrawReason::INSUFFICIENT_MATERIAL) == "insufficient material");
        CHECK(describe_draw(DrawReason::THREEFOLD_REPETITION) == "threefold repetition");
        CHECK(describe_draw(DrawReason::FIFTY_MOVES) == "50-move rule");
    }
} // namespace

int main()
{
    TestInsufficientMaterial();
    TestRepetition();
    TestRepetitionAfterPawnMove();
    TestRepetitionInVariation();
    TestFindDraw();
    TestTexts();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
