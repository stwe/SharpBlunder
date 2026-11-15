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

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>
#include "ui/BoardView.h"

namespace
{
    constexpr sf::Color LIGHT_SQUARE{ 240, 217, 181 };
    constexpr sf::Color DARK_SQUARE{ 181, 136, 99 };

    constexpr sf::Color LAST_MOVE_COLOR{ 205, 210, 106, 150 };
    constexpr sf::Color SELECTED_COLOR{ 246, 246, 105, 170 };
    constexpr sf::Color CHECK_COLOR{ 220, 40, 40, 170 };
    constexpr sf::Color HINT_COLOR{ 40, 160, 70, 150 };
    constexpr sf::Color PICKER_COLOR{ 30, 30, 30, 240 };
    constexpr sf::Color ARROW_COLOR{ 20, 110, 190, 190 };

    constexpr float DRAGGED_PIECE_SCALE{ 1.1f };

    // The arrow of the best move, as a part of the square size.
    constexpr float ARROW_SHAFT_WIDTH{ 0.17f };
    constexpr float ARROW_HEAD_LENGTH{ 0.4f };
    constexpr float ARROW_HEAD_WIDTH{ 0.5f };

    // The promotion picker is a bar over the four middle files, 3.5 squares below the top of the board.
    constexpr float PICKER_FIRST_FILE{ 2.0f };
    constexpr float PICKER_TOP{ 3.5f };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::BoardView::BoardView(const sf::Vector2f t_origin, const float t_squareSize, const PieceSet& t_pieces)
    : m_origin(t_origin)
    , m_squareSize(t_squareSize)
    , m_pieces(t_pieces)
{
}

//-------------------------------------------------
// Geometry
//-------------------------------------------------

std::optional<int> sg::chess::ui::BoardView::SquareAt(const sf::Vector2f t_position) const
{
    const float x{ t_position.x - m_origin.x };
    const float y{ t_position.y - m_origin.y };
    const float boardSize{ static_cast<float>(core::BOARD_SIZE) * m_squareSize };

    if (x < 0.0f || y < 0.0f || x >= boardSize || y >= boardSize)
    {
        return std::nullopt;
    }

    const int column{ static_cast<int>(x / m_squareSize) };
    const int row{ static_cast<int>(y / m_squareSize) };

    const int file{ m_flipped ? core::BOARD_SIZE - 1 - column : column };
    const int rank{ m_flipped ? row : core::BOARD_SIZE - 1 - row };

    return core::square_index(file, rank);
}

sf::FloatRect sg::chess::ui::BoardView::PromotionRect(const std::size_t t_index) const
{
    const sf::Vector2f position{ m_origin.x + (PICKER_FIRST_FILE + static_cast<float>(t_index)) * m_squareSize,
                                 m_origin.y + PICKER_TOP * m_squareSize };

    return sf::FloatRect(position, { m_squareSize, m_squareSize });
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::BoardView::Draw(sf::RenderTarget& t_target, const core::Position& t_position, const BoardOverlay& t_overlay) const
{
    DrawSquares(t_target);
    DrawHighlights(t_target, t_position, t_overlay);
    DrawPieces(t_target, t_position, t_overlay);

    if (t_overlay.bestMove)
    {
        DrawArrow(t_target, *t_overlay.bestMove);
    }

    if (t_overlay.promotionColor)
    {
        DrawPromotionPicker(t_target, *t_overlay.promotionColor);
    }
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

sf::Vector2f sg::chess::ui::BoardView::SquarePosition(const int t_square) const
{
    const int file{ core::file_of(t_square) };
    const int rank{ core::rank_of(t_square) };

    const int column{ m_flipped ? core::BOARD_SIZE - 1 - file : file };
    const int row{ m_flipped ? rank : core::BOARD_SIZE - 1 - rank };

    return { m_origin.x + static_cast<float>(column) * m_squareSize,
             m_origin.y + static_cast<float>(row) * m_squareSize };
}

void sg::chess::ui::BoardView::DrawSquares(sf::RenderTarget& t_target) const
{
    sf::RectangleShape square({ m_squareSize, m_squareSize });

    for (int index{ 0 }; index < core::SQUARE_COUNT; ++index)
    {
        // a1 (file 0, rank 0) is a dark square
        const bool dark{ (core::file_of(index) + core::rank_of(index)) % 2 == 0 };

        square.setFillColor(dark ? DARK_SQUARE : LIGHT_SQUARE);
        square.setPosition(SquarePosition(index));
        t_target.draw(square);
    }
}

void sg::chess::ui::BoardView::DrawHighlights(sf::RenderTarget& t_target, const core::Position& t_position,
                                              const BoardOverlay& t_overlay) const
{
    if (t_overlay.lastMove)
    {
        Tint(t_target, t_overlay.lastMove->from, LAST_MOVE_COLOR);
        Tint(t_target, t_overlay.lastMove->to, LAST_MOVE_COLOR);
    }

    if (t_overlay.checkedSquare)
    {
        Tint(t_target, *t_overlay.checkedSquare, CHECK_COLOR);
    }

    if (t_overlay.selectedSquare)
    {
        Tint(t_target, *t_overlay.selectedSquare, SELECTED_COLOR);
    }

    // A dot on an empty target square, a ring around a piece that would be captured.
    for (const int target : t_overlay.targetSquares)
    {
        const bool isCapture{ t_position.At(target).has_value() };
        const float radius{ isCapture ? m_squareSize * 0.45f : m_squareSize * 0.15f };

        sf::CircleShape hint(radius);
        hint.setOrigin({ radius, radius });
        hint.setPosition(SquarePosition(target) + sf::Vector2f(m_squareSize / 2.0f, m_squareSize / 2.0f));

        if (isCapture)
        {
            hint.setFillColor(sf::Color::Transparent);
            hint.setOutlineThickness(-m_squareSize * 0.075f);
            hint.setOutlineColor(HINT_COLOR);
        }
        else
        {
            hint.setFillColor(HINT_COLOR);
        }

        t_target.draw(hint);
    }
}

void sg::chess::ui::BoardView::DrawPieces(sf::RenderTarget& t_target, const core::Position& t_position,
                                          const BoardOverlay& t_overlay) const
{
    for (int index{ 0 }; index < core::SQUARE_COUNT; ++index)
    {
        const auto piece{ t_position.At(index) };
        if (!piece)
        {
            continue;
        }

        // The dragged piece is drawn under the mouse cursor instead.
        if (t_overlay.dragged && t_overlay.selectedSquare == index)
        {
            continue;
        }

        DrawPiece(t_target, *piece, SquarePosition(index), m_squareSize);
    }

    if (t_overlay.dragged)
    {
        const float size{ m_squareSize * DRAGGED_PIECE_SCALE };
        const sf::Vector2f topLeft{ t_overlay.dragged->position - sf::Vector2f(size / 2.0f, size / 2.0f) };

        DrawPiece(t_target, t_overlay.dragged->piece, topLeft, size);
    }
}

void sg::chess::ui::BoardView::DrawArrow(sf::RenderTarget& t_target, const core::Move& t_move) const
{
    const sf::Vector2f half{ m_squareSize / 2.0f, m_squareSize / 2.0f };
    const sf::Vector2f start{ SquarePosition(t_move.from) + half };
    const sf::Vector2f end{ SquarePosition(t_move.to) + half };

    const sf::Vector2f delta{ end - start };
    const float length{ std::hypot(delta.x, delta.y) };
    if (length < 1.0f)
    {
        return;
    }

    const sf::Vector2f direction{ delta / length };
    const sf::Vector2f side{ -direction.y, direction.x };

    const float headLength{ ARROW_HEAD_LENGTH * m_squareSize };
    const float headHalfWidth{ ARROW_HEAD_WIDTH * m_squareSize / 2.0f };
    const float shaftWidth{ ARROW_SHAFT_WIDTH * m_squareSize };

    // The tip stops a little before the center of the target square, so a captured piece stays visible.
    const sf::Vector2f tip{ end - direction * (m_squareSize * 0.1f) };
    const sf::Vector2f base{ tip - direction * headLength };

    // SFML draws convex shapes only, and an arrow is not convex: it is a shaft and a head, which touch at the
    // base of the head but do not overlap. So the translucent color is the same everywhere.
    sf::ConvexShape shaft(4);
    shaft.setPoint(0, start + side * (shaftWidth / 2.0f));
    shaft.setPoint(1, base + side * (shaftWidth / 2.0f));
    shaft.setPoint(2, base - side * (shaftWidth / 2.0f));
    shaft.setPoint(3, start - side * (shaftWidth / 2.0f));
    shaft.setFillColor(ARROW_COLOR);
    t_target.draw(shaft);

    sf::ConvexShape head(3);
    head.setPoint(0, base + side * headHalfWidth);
    head.setPoint(1, tip);
    head.setPoint(2, base - side * headHalfWidth);
    head.setFillColor(ARROW_COLOR);
    t_target.draw(head);
}

void sg::chess::ui::BoardView::DrawPromotionPicker(sf::RenderTarget& t_target, const core::Color t_color) const
{
    const sf::FloatRect first{ PromotionRect(0) };

    sf::RectangleShape background({ static_cast<float>(PROMOTION_TYPES.size()) * m_squareSize, m_squareSize });
    background.setPosition(first.position);
    background.setFillColor(PICKER_COLOR);
    t_target.draw(background);

    for (std::size_t i{ 0 }; i < PROMOTION_TYPES.size(); ++i)
    {
        DrawPiece(t_target, core::Piece{ PROMOTION_TYPES[i], t_color }, PromotionRect(i).position, m_squareSize);
    }
}

void sg::chess::ui::BoardView::Tint(sf::RenderTarget& t_target, const int t_square, const sf::Color t_color) const
{
    sf::RectangleShape square({ m_squareSize, m_squareSize });
    square.setPosition(SquarePosition(t_square));
    square.setFillColor(t_color);
    t_target.draw(square);
}

void sg::chess::ui::BoardView::DrawPiece(sf::RenderTarget& t_target, const core::Piece t_piece,
                                         const sf::Vector2f t_topLeft, const float t_size) const
{
    const sf::Texture& texture{ m_pieces.Get(t_piece) };
    const float scale{ t_size / static_cast<float>(texture.getSize().x) };

    sf::Sprite sprite(texture);
    sprite.setScale({ scale, scale });
    sprite.setPosition(t_topLeft);
    t_target.draw(sprite);
}
