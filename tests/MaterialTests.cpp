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
#include "Check.h"
#include "core/Material.h"
#include "core/Position.h"

namespace
{
    using sg::chess::core::Color;
    using sg::chess::core::Move;
    using sg::chess::core::PieceType;
    using sg::chess::core::Position;

    void TestPoints()
    {
        using sg::chess::core::piece_points;

        CHECK(piece_points(PieceType::PAWN) == 1);
        CHECK(piece_points(PieceType::KNIGHT) == 3);
        CHECK(piece_points(PieceType::BISHOP) == 3);
        CHECK(piece_points(PieceType::ROOK) == 5);
        CHECK(piece_points(PieceType::QUEEN) == 9);
        CHECK(piece_points(PieceType::KING) == 0);

        sg::chess::core::Material material;
        material.byWhite.count[static_cast<std::size_t>(PieceType::QUEEN)] = 1;
        material.byWhite.count[static_cast<std::size_t>(PieceType::PAWN)] = 2;
        material.byBlack.count[static_cast<std::size_t>(PieceType::ROOK)] = 1;

        CHECK(material.byWhite.Points() == 11);
        CHECK(material.byBlack.Points() == 5);
        CHECK(material.Balance() == 6);
    }

    // The piece that a move captures, as text ("bP" for a Black pawn), or "none".
    std::string Captured(const char* t_fen, const char* t_uci)
    {
        const auto piece{ sg::chess::core::captured_piece(Position::FromFen(t_fen), *Move::FromUci(t_uci)) };
        if (!piece)
        {
            return "none";
        }

        return std::string{ piece->color == Color::WHITE ? "w" : "b" } +
               std::string{ sg::chess::core::PIECE_LETTERS[static_cast<std::size_t>(piece->type)] };
    }

    void TestCaptures()
    {
        // A pawn takes a pawn, a quiet move, an own piece on the target square is no capture.
        CHECK(Captured("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1", "e4d5") == "bP");
        CHECK(Captured("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1", "e4e5") == "none");
        CHECK(Captured("4k3/8/8/8/8/8/8/R2QK3 w - - 0 1", "a1d1") == "none");

        // Black captures a White piece.
        CHECK(Captured("4k3/8/8/8/8/8/8/r2NK3 b - - 0 1", "a1d1") == "wN");
        CHECK(Captured("4k3/8/8/8/3n4/8/4Q3/4K3 b - - 0 1", "d4e2") == "wQ");

        // A promotion that takes a piece.
        CHECK(Captured("k1r5/1P6/8/8/8/8/8/K7 w - - 0 1", "b7c8q") == "bR");
        CHECK(Captured("k7/1P6/8/8/8/8/8/K7 w - - 0 1", "b7b8q") == "none");

        // En passant: the captured pawn is not on the target square.
        CHECK(Captured("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", "e5d6") == "bP");
        CHECK(Captured("4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1", "e4d3") == "wP");

        // Castling moves the king, it captures nothing.
        CHECK(Captured("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1g1") == "none");

        // No piece on the start square: nothing is captured.
        CHECK(Captured("4k3/8/8/8/8/8/8/4K3 w - - 0 1", "a1a2") == "none");
    }
}

int main()
{
    TestPoints();
    TestCaptures();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
