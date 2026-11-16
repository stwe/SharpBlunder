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

#include "core/MoveQuality.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // Icons
    //-------------------------------------------------

    // Small icons for the side panel. They are drawn with Dear ImGui shapes, so they need no image files and
    // no special glyphs in the font. Call them while a GUI frame is being built.

    /**
     * @brief Draws a colored dot for the quality of a move: green, yellow or red.
     *
     * The dot is a square item of the given size in the current line. A tooltip explains the color.
     * The caller must have pushed an ID if several dots are shown.
     *
     * @param t_quality The quality of the move.
     * @param t_size The edge length of the item in pixels, usually the height of a button.
     */
    void quality_dot(core::MoveQuality t_quality, float t_size);

    /**
     * @brief Draws a button with a magnifying glass.
     *
     * The caller must have pushed an ID if several buttons are shown.
     *
     * @param t_active True if the details the button opens are shown: the glass is drawn in the accent color.
     * @param t_size The edge length of the button in pixels, usually the height of a button.
     * @param t_tooltip The text shown when the mouse rests on the button.
     *
     * @return True in the frame in which the button was clicked.
     */
    [[nodiscard]] bool magnifier_button(bool t_active, float t_size, const char* t_tooltip);
} // namespace sg::chess::ui
