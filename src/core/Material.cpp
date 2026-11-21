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
#include "core/Material.h"

int sg::chess::core::CapturedPieces::Points() const noexcept
{
    int points{ 0 };

    for (std::size_t type{ 0 }; type < count.size(); ++type)
    {
        points += count[type] * piece_points(static_cast<PieceType>(type));
    }

    return points;
}

int sg::chess::core::piece_points(const PieceType t_type) noexcept
{
    switch (t_type)
    {
    case PieceType::QUEEN:
        return 9;
    case PieceType::ROOK:
        return 5;
    case PieceType::BISHOP:
    case PieceType::KNIGHT:
        return 3;
    case PieceType::PAWN:
        return 1;
    case PieceType::KING:
        break;
    }

    return 0;
}

std::optional<sg::chess::core::Piece> sg::chess::core::captured_piece(const Position& t_before, const Move& t_move)
{
    const auto mover{ t_before.At(t_move.from) };
    if (!mover)
    {
        return std::nullopt;
    }

    if (const auto target{ t_before.At(t_move.to) }; target)
    {
        return target->color != mover->color ? target : std::nullopt;
    }

    // En passant: a pawn goes to another file, but there is nothing on its target square.
    if (mover->type == PieceType::PAWN && file_of(t_move.from) != file_of(t_move.to))
    {
        return Piece{ PieceType::PAWN, mover->color == Color::WHITE ? Color::BLACK : Color::WHITE };
    }

    return std::nullopt;
}

sg::chess::core::Material sg::chess::core::captured_material(const GameState& t_game)
{
    Material material;

    // From the current position back to the start: every move on the way is part of the game up to here.
    for (std::optional<GameState::NodeId> id{ t_game.CurrentId() }; id; id = t_game.At(*id).parent)
    {
        const GameState::Node& node{ t_game.At(*id) };
        if (!node.move)
        {
            continue;
        }

        const auto captured{ captured_piece(t_game.At(*node.parent).info.position, *node.move) };
        if (!captured)
        {
            continue;
        }

        CapturedPieces& side{ captured->color == Color::BLACK ? material.byWhite : material.byBlack };
        ++side.count[static_cast<std::size_t>(captured->type)];
    }

    return material;
}
