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

        [[nodiscard]] std::optional<int> SelectedSquare() const noexcept { return m_selected; }
        [[nodiscard]] bool IsDragging() const noexcept { return m_dragging; }
        [[nodiscard]] sf::Vector2f CursorPosition() const noexcept { return m_cursor; }
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
            int from;
            int to;
        };

        //-------------------------------------------------
        // Helper
        //-------------------------------------------------

        [[nodiscard]] std::optional<core::Move> OnPress(sf::Vector2f t_position, const core::PositionInfo& t_info);
        [[nodiscard]] std::optional<core::Move> OnRelease(sf::Vector2f t_position, const core::PositionInfo& t_info);

        /**
         * @brief Completes a move from one square to another.
         *
         * @return The move, or std::nullopt if the move is not legal or if a promotion piece must be chosen first.
         */
        [[nodiscard]] std::optional<core::Move> TryMove(const core::PositionInfo& t_info, int t_from, int t_to);

        [[nodiscard]] static bool HasMove(const core::PositionInfo& t_info, int t_from, int t_to);
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
