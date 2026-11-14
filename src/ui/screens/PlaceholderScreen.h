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

#include <string>
#include <string_view>
#include "ui/Screen.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // PlaceholderScreen
    //-------------------------------------------------

    /**
     * @brief An empty page that only shows a title and a short description.
     *
     * Stands in for pages that are not implemented yet. Once a page gets real content,
     * it becomes a class of its own and the placeholder is dropped.
     */
    class PlaceholderScreen final : public Screen
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates a placeholder page.
         *
         * @param t_title The title, also used as the name in the navigation bar.
         * @param t_description What this page will do later.
         */
        PlaceholderScreen(std::string t_title, std::string t_description);

        PlaceholderScreen(const PlaceholderScreen& t_other) = delete;
        PlaceholderScreen(PlaceholderScreen&& t_other) noexcept = delete;
        PlaceholderScreen& operator=(const PlaceholderScreen& t_other) = delete;
        PlaceholderScreen& operator=(PlaceholderScreen&& t_other) noexcept = delete;

        ~PlaceholderScreen() noexcept override = default;

        //-------------------------------------------------
        // Screen
        //-------------------------------------------------

        /**
         * @brief Returns the name shown in the navigation bar.
         *
         * @return The title that was given to the constructor.
         */
        [[nodiscard]] std::string_view Title() const noexcept override;

        /**
         * @brief Shows the title and the description on a page that fills the area to the right of the navigation bar.
         */
        void DrawGui() override;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::string m_title;        ///< Title of the page
        std::string m_description;  ///< Text shown on the page
    };
}
