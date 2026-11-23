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

#include <SFML/Graphics/Texture.hpp>
#include "core/MoveQuality.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // Icons
    //-------------------------------------------------

    // Small icons for the side panel. Call them while a GUI frame is being built.
    // They draw on the draw list of the current window, so they do not change the layout.

    /**
     * @brief Describes a move quality in words, for a tooltip.
     *
     * @param t_quality The quality.
     *
     * @return The text.
     */
    [[nodiscard]] const char* describe_quality(core::MoveQuality t_quality);

    /**
     * @brief Draws a colored dot for the quality of a move: green, yellow or red.
     *
     * @param t_x The window position of the left edge of the square the dot is centered in.
     * @param t_y The window position of its top edge.
     * @param t_size The edge length of the square in pixels.
     * @param t_quality The quality of the move.
     */
    void draw_quality_dot(float t_x, float t_y, float t_size, core::MoveQuality t_quality);

    /**
     * @brief Draws a chess piece as a small icon, on a light square. That way black pieces stand out
     * from the dark panel.
     *
     * @param t_texture The image of the piece, e.g. from the PieceSet of the board.
     * @param t_x The window position of the left edge of the icon.
     * @param t_y The window position of its top edge.
     * @param t_size The edge length of the icon in pixels.
     */
    void draw_piece_icon(const sf::Texture& t_texture, float t_x, float t_y, float t_size);

    /**
     * @brief Draws a button with a magnifying glass.
     *
     * The caller must have pushed an ID if several buttons are shown. The button is a square item of the given
     * size in the current line.
     *
     * @param t_active True if the details the button opens are shown: the glass is drawn in the accent color.
     * @param t_size The edge length of the button in pixels, usually the height of a button.
     * @param t_tooltip The text shown when the mouse rests on the button.
     *
     * @return True in the frame in which the button was clicked.
     */
    [[nodiscard]] bool magnifier_button(bool t_active, float t_size, const char* t_tooltip);

    /**
     * @brief Draws a button with a speech bubble: the sign for a comment.
     *
     * The caller must have pushed an ID if several buttons are shown. The button is a square item of the given
     * size in the current line.
     *
     * @param t_filled True if there is a comment: the bubble is filled. Otherwise only its outline is drawn,
     *                 faint, as an invitation to write one.
     * @param t_active True if the comment is shown in full: the bubble gets a light outline.
     * @param t_size The edge length of the button in pixels, usually the height of a line of text.
     * @param t_color The color of a filled bubble, as ABGR (see IM_COL32).
     *
     * @return True in the frame in which the button was clicked.
     */
    [[nodiscard]] bool comment_icon(bool t_filled, bool t_active, float t_size, unsigned int t_color);

    /**
     * @brief Draws a button with a pencil: the sign for editing.
     *
     * The caller must have pushed an ID if several buttons are shown. The button is a square item of the given
     * size in the current line.
     *
     * @param t_size The edge length of the button in pixels.
     * @param t_tooltip The text shown when the mouse rests on the button.
     *
     * @return True in the frame in which the button was clicked.
     */
    [[nodiscard]] bool pencil_button(float t_size, const char* t_tooltip);
} // namespace sg::chess::ui
