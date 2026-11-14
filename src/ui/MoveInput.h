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

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <optional>
#include <vector>
#include "core/Move.h"
#include "core/PositionInfo.h"
#include "ui/BoardView.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // MoveInput
    //-------------------------------------------------

    /**
     * @brief Turns mouse input on the board into moves.
     *
     * A move can be entered in two ways: press a piece, drag it to the target square and release,
     * or click the piece and then click the target square. A right click cancels.
     * For a pawn promotion the user is asked which piece to promote to.
     *
     * MoveInput only collects input and remembers the state of the interaction (selection, dragging).
     * It does not play the move. It returns it, and the caller decides what to do with it.
     */
    class MoveInput
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the input handler.
         *
         * @param t_view The board to read the mouse positions from. Not owned: it must outlive this object.
         */
        explicit MoveInput(const BoardView& t_view);

        MoveInput(const MoveInput& t_other) = delete;
        MoveInput(MoveInput&& t_other) noexcept = delete;
        MoveInput& operator=(const MoveInput& t_other) = delete;
        MoveInput& operator=(MoveInput&& t_other) noexcept = delete;

        ~MoveInput() noexcept = default;

        //-------------------------------------------------
        // Input
        //-------------------------------------------------

        /**
         * @brief Handles one event.
         *
         * @param t_event The event. Only mouse events are used.
         * @param t_info The current position with its legal moves.
         *
         * @return The move the user has entered, or std::nullopt if the move is not complete yet.
         */
        [[nodiscard]] std::optional<core::Move> HandleEvent(const sf::Event& t_event, const core::PositionInfo& t_info);

        /**
         * @brief Forgets the selection. Call it whenever the position on the board changes.
         */
        void Reset() noexcept;

        //-------------------------------------------------
        // State (for drawing)
        //-------------------------------------------------

        /**
         * @brief Returns the square of the selected piece.
         *
         * @return The square, or std::nullopt if no piece is selected.
         */
        [[nodiscard]] std::optional<int> SelectedSquare() const noexcept { return m_selected; }

        /**
         * @brief Tells whether a piece is being dragged with the mouse.
         *
         * @return True while the left mouse button is held down on a piece.
         */
        [[nodiscard]] bool IsDragging() const noexcept { return m_dragging; }

        /**
         * @brief Returns the last known mouse position.
         *
         * @return The position in window coordinates.
         */
        [[nodiscard]] sf::Vector2f CursorPosition() const noexcept { return m_cursor; }

        /**
         * @brief Tells whether the user has to choose the promotion piece.
         *
         * @return True while the promotion picker is shown.
         */
        [[nodiscard]] bool IsPromotionPending() const noexcept { return m_promotion.has_value(); }

        /**
         * @brief Returns the squares the selected piece can move to.
         *
         * @param t_info The current position with its legal moves.
         *
         * @return The target squares, empty if no piece is selected.
         */
        [[nodiscard]] std::vector<int> TargetSquares(const core::PositionInfo& t_info) const;

    private:
        //-------------------------------------------------
        // Types
        //-------------------------------------------------

        /**
         * @brief A pawn move waiting for the choice of the promotion piece.
         */
        struct PendingPromotion
        {
            int from; ///< The start square of the pawn
            int to; ///< The target square of the pawn
        };

        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        /**
         * @brief Handles a press of the left mouse button: chooses the promotion piece, completes a move by a second
         * click, or selects a piece.
         *
         * @param t_position The mouse position in window coordinates.
         * @param t_info The current position with its legal moves.
         *
         * @return The completed move, or std::nullopt if the move is not complete yet.
         */
        [[nodiscard]] std::optional<core::Move> OnPress(sf::Vector2f t_position, const core::PositionInfo& t_info);

        /**
         * @brief Handles the release of the left mouse button: completes a drag and drop move or cancels the selection.
         *
         * @param t_position The mouse position in window coordinates.
         * @param t_info The current position with its legal moves.
         *
         * @return The completed move, or std::nullopt if the move is not complete yet.
         */
        [[nodiscard]] std::optional<core::Move> OnRelease(sf::Vector2f t_position, const core::PositionInfo& t_info);

        /**
         * @brief Completes a move from one square to another.
         *
         * @param t_info The current position with its legal moves.
         * @param t_from The start square.
         * @param t_to The target square.
         *
         * @return The move, or std::nullopt if the move is not legal or if a promotion piece must be chosen first.
         */
        [[nodiscard]] std::optional<core::Move> TryMove(const core::PositionInfo& t_info, int t_from, int t_to);

        /**
         * @brief Tells whether a legal move leads from one square to another.
         *
         * @param t_info The current position with its legal moves.
         * @param t_from The start square.
         * @param t_to The target square.
         *
         * @return True if there is at least one such move (a promotion has four).
         */
        [[nodiscard]] static bool HasMove(const core::PositionInfo& t_info, int t_from, int t_to);

        /**
         * @brief Tells whether a square holds a piece of the side to move.
         *
         * @param t_info The current position with its legal moves.
         * @param t_square The square index.
         *
         * @return True if the piece on the square belongs to the side to move.
         */
        [[nodiscard]] static bool IsOwnPiece(const core::PositionInfo& t_info, int t_square);

        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        const BoardView& m_view;                      ///< The board (not owned)
        std::optional<int> m_selected;                ///< Square of the selected piece
        bool m_dragging{ false };                     ///< True while the mouse button is held down on the piece
        sf::Vector2f m_cursor;                        ///< Last known mouse position
        std::optional<PendingPromotion> m_promotion;  ///< Set while waiting for the promotion choice
    };
}
