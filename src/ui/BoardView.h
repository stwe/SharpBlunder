#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include "core/Position.h"
#include "ui/PieceSet.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // BoardView
    //-------------------------------------------------

    /**
     * @brief Draws a chess board and the pieces of a Position.
     *
     * The view only knows how to draw. The position is passed in on every Draw() call,
     * so the same view can show any position. White is at the bottom.
     */
    class BoardView
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates a board view.
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
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Draws the squares and then the pieces on top of them.
         *
         * @param t_target The surface to draw on.
         * @param t_position The position to show.
         */
        void Draw(sf::RenderTarget& t_target, const core::Position& t_position) const;

    private:
        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Returns the window position of the top left corner of a square.
         *
         * @param t_file The file, 0 to 7.
         * @param t_rank The rank, 0 to 7 (rank 0 is drawn at the bottom).
         *
         * @return The position in pixels.
         */
        [[nodiscard]] sf::Vector2f SquarePosition(int t_file, int t_rank) const;

        void DrawSquares(sf::RenderTarget& t_target) const;
        void DrawPieces(sf::RenderTarget& t_target, const core::Position& t_position) const;

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        sf::Vector2f m_origin;       ///< Top left corner of the board in the window
        float m_squareSize;          ///< Edge length of one square in pixels
        const PieceSet& m_pieces;    ///< The piece images (not owned)
    };
}
