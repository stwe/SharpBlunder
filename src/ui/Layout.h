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

namespace sg::chess::ui
{
    //-------------------------------------------------
    // Layout constants
    //-------------------------------------------------

    /**
     * @brief Width of the navigation bar on the left side of the window in pixels.
     *
     * The area to the right of it belongs to the current screen.
     */
    inline constexpr float NAVIGATION_WIDTH{ 180.0f };

    /**
     * @brief The space between the top edge of the window and the first panel, in pixels.
     *
     * The side panel of the analysis page and the navigation bar start at this height. The strip above the board
     * does, too.
     */
    inline constexpr float PAGE_MARGIN_TOP{ 14.0f };

    /**
     * @brief The space between the lower edge of the panels and the bottom edge of the window, in pixels.
     *
     * It is the space that the side panel of the analysis page has below it (it ends at the lower strip of the board).
     * The navigation bar ends at the same height, so that both look the same.
     */
    inline constexpr float PAGE_MARGIN_BOTTOM{ 12.0f };

    /**
     * @brief The space between the left edge of the window and the navigation bar, in pixels.
     *
     * The navigation bar still ends at NAVIGATION_WIDTH, so the pages to its right do not move.
     */
    inline constexpr float NAVIGATION_MARGIN_LEFT{ 14.0f };
}
