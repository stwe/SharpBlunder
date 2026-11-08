#pragma once

#include <SFML/System/Vector2.hpp>
#include <optional>
#include <vector>
#include "core/Move.h"
#include "core/Piece.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // BoardOverlay
    //-------------------------------------------------

    /**
     * @brief Everything the board shows on top of the plain position: highlights, hints, a dragged piece.
     *
     * The screen fills this in every frame from the game and from the mouse input. The BoardView only draws it.
     */
    struct BoardOverlay
    {
        /**
         * @brief A piece that is currently dragged with the mouse.
         */
        struct DraggedPiece
        {
            core::Piece piece;      ///< The piece
            sf::Vector2f position;  ///< Center of the piece, in window coordinates
        };

        std::optional<core::Move> lastMove;         ///< The previous move: start and target square are highlighted
        std::optional<int> checkedSquare;           ///< Square of the king that is in check
        std::optional<int> selectedSquare;          ///< Square of the selected piece
        std::vector<int> targetSquares;             ///< Squares the selected piece can move to
        std::optional<DraggedPiece> dragged;        ///< The dragged piece; its square shows no piece meanwhile
        std::optional<core::Color> promotionColor;  ///< If set, the promotion picker is shown for this side
    };
}
