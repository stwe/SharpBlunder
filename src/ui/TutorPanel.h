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

namespace sg::chess::ui
{
    //-------------------------------------------------
    // TutorPanel
    //-------------------------------------------------

    /**
     * @brief A box in the side panel of the analysis page for the comments of the tutor.
     *
     * For now it only shows a text that it is given. Later a program part with a language model will write the
     * texts. The box does not know where a text comes from: whoever has one calls SetText().
     */
    class TutorPanel
    {
    public:
        //-------------------------------------------------
        // Constants
        //-------------------------------------------------

        /**
         * @brief The height of the box in pixels, with the title.
         */
        static constexpr float HEIGHT{ 96.0f };

        //-------------------------------------------------
        // Text
        //-------------------------------------------------

        /**
         * @brief Sets the text that the box shows.
         *
         * @param t_text The text. An empty text shows the hint that there is no comment.
         */
        void SetText(std::string t_text);

        /**
         * @brief Tells whether the box has a comment.
         *
         * @return True if a text was set and is not empty.
         */
        [[nodiscard]] bool HasText() const noexcept;

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Draws the box at the current position of the window. It is as wide as the window.
         */
        void Draw() const;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        std::string m_text; ///< The comment, or empty
    };
} // namespace sg::chess::ui
