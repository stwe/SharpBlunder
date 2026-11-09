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

#include "ui/ScreenManager.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // NavigationBar
    //-------------------------------------------------

    /**
     * @brief The menu on the left side of the window: one entry per screen.
     *
     * The bar has no state of its own. It reads the screens from the ScreenManager
     * and asks it to switch when an entry is clicked.
     */
    class NavigationBar
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        NavigationBar() = default;

        NavigationBar(const NavigationBar& t_other) = delete;
        NavigationBar(NavigationBar&& t_other) noexcept = delete;
        NavigationBar& operator=(const NavigationBar& t_other) = delete;
        NavigationBar& operator=(NavigationBar&& t_other) noexcept = delete;

        ~NavigationBar() noexcept = default;

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Builds the navigation bar with Dear ImGui. Call it between GuiLayer::BeginFrame() and Render().
         *
         * @param t_screens The screens to list. The clicked one becomes the active screen.
         */
        void DrawGui(ScreenManager& t_screens) const;
    };
}
