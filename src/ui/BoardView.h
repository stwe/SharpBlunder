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

        void DrawSquares(sf::RenderTarget& t_target) const;
        void DrawHighlights(sf::RenderTarget& t_target, const core::Position& t_position, const BoardOverlay& t_overlay) const;
        void DrawPieces(sf::RenderTarget& t_target, const core::Position& t_position, const BoardOverlay& t_overlay) const;
        void DrawPromotionPicker(sf::RenderTarget& t_target, core::Color t_color) const;

        void Tint(sf::RenderTarget& t_target, int t_square, sf::Color t_color) const;
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
