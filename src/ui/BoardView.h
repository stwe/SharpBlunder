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

#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <optional>
#include "core/Piece.h"
#include "core/Position.h"
#include "ui/BoardOverlay.h"
#include "ui/PieceSet.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // BoardView
    //-------------------------------------------------

    /**
     * @brief Draws a chess board and the pieces of a Position, and translates between squares and window positions.
     *
     * The view only knows how to draw. The position and the overlay are passed in on every Draw() call,
     * so the same view can show any position.
     */
    class BoardView
    {
    public:
        //-------------------------------------------------
        // Constants
        //-------------------------------------------------

        /**
         * @brief The pieces offered when a pawn is promoted, in the order they are shown.
         */
        static constexpr std::array<core::PieceType, 4> PROMOTION_TYPES{ core::PieceType::QUEEN,
                                                                         core::PieceType::ROOK,
                                                                         core::PieceType::BISHOP,
                                                                         core::PieceType::KNIGHT };

        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates a board view. White is at the bottom until the board is flipped.
         *
         * @param t_origin The window position of the top left corner of the board.
         * @param t_squareSize The edge length of one square in pixels.
         * @param t_pieces The piece images. Not owned: the PieceSet must outlive this view.
         */
        BoardView(sf::Vector2f t_origin, float t_squareSize, const PieceSet& t_pieces);

        BoardView(const BoardView& t_other) = delete;
        BoardView(BoardView&& t_other) noexcept = delete;
        BoardView& operator=(const BoardView& t_other) = delete;
        BoardView& operator=(BoardView&& t_other) noexcept = delete;

        ~BoardView() noexcept = default;

        //-------------------------------------------------
        // Orientation
        //-------------------------------------------------

        /**
         * @brief Turns the board around: Black is at the bottom afterwards (and the other way round).
         */
        void Flip() noexcept { m_flipped = !m_flipped; }

        /**
         * @brief Tells whether the board is turned around.
         *
         * @return True if Black is at the bottom.
         */
        [[nodiscard]] bool IsFlipped() const noexcept { return m_flipped; }

        //-------------------------------------------------
        // Geometry
        //-------------------------------------------------

        /**
         * @brief Finds the square at a window position.
         *
         * @param t_position A position in window coordinates.
         *
         * @return The square index, or std::nullopt if the position is outside the board.
         */
        [[nodiscard]] std::optional<int> SquareAt(sf::Vector2f t_position) const;

        /**
         * @brief Returns the clickable area of one of the promotion pieces.
         *
         * @param t_index Index into PROMOTION_TYPES.
         *
         * @return The area in window coordinates.
         */
        [[nodiscard]] sf::FloatRect PromotionRect(std::size_t t_index) const;

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Draws the squares, the highlights, the pieces and finally the dragged piece and the promotion picker.
         *
         * @param t_target The surface to draw on.
         * @param t_position The position to show.
         * @param t_overlay Highlights and hints to show on top.
         */
        void Draw(sf::RenderTarget& t_target, const core::Position& t_position, const BoardOverlay& t_overlay = {}) const;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Returns the window position of the top left corner of a square.
         *
         * @param t_square The square index.
         *
         * @return The position in pixels.
         */
        [[nodiscard]] sf::Vector2f SquarePosition(int t_square) const;

        /**
         * @brief Draws the 64 squares.
         *
         * @param t_target The surface to draw on.
         */
        void DrawSquares(sf::RenderTarget& t_target) const;

        /**
         * @brief Draws the highlights: last move, check, selection, and the hints for the target squares of the
         * selected piece.
         *
         * @param t_target The surface to draw on.
         * @param t_position The position. It tells a capture from a quiet move.
         * @param t_overlay The highlights to draw.
         */
        void DrawHighlights(sf::RenderTarget& t_target, const core::Position& t_position,
                            const BoardOverlay& t_overlay) const;

        /**
         * @brief Draws the pieces. A dragged piece is drawn last, under the mouse cursor.
         *
         * @param t_target The surface to draw on.
         * @param t_position The position.
         * @param t_overlay Tells which piece is dragged and where.
         */
        void DrawPieces(sf::RenderTarget& t_target, const core::Position& t_position,
                        const BoardOverlay& t_overlay) const;

        /**
         * @brief Draws the bar for choosing the promotion piece.
         *
         * @param t_target The surface to draw on.
         * @param t_color The side that promotes. The pieces are shown in this color.
         */
        void DrawPromotionPicker(sf::RenderTarget& t_target, core::Color t_color) const;

        /**
         * @brief Covers a square with a translucent color.
         *
         * @param t_target The surface to draw on.
         * @param t_square The square index.
         * @param t_color The color.
         */
        void Tint(sf::RenderTarget& t_target, int t_square, sf::Color t_color) const;

        /**
         * @brief Draws a piece, scaled to a given size.
         *
         * @param t_target The surface to draw on.
         * @param t_piece The piece.
         * @param t_topLeft The top left corner of the piece in window coordinates.
         * @param t_size The edge length of the piece in pixels.
         */
        void DrawPiece(sf::RenderTarget& t_target, core::Piece t_piece, sf::Vector2f t_topLeft, float t_size) const;

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        sf::Vector2f m_origin;     ///< Top left corner of the board in the window
        float m_squareSize;        ///< Edge length of one square in pixels
        const PieceSet& m_pieces;  ///< The piece images (not owned)
        bool m_flipped{ false };   ///< True: Black is at the bottom
    };
}
