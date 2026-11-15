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

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>

namespace sg::chess::ui
{
    //-------------------------------------------------
    // EvalBar
    //-------------------------------------------------

    /**
     * @brief The evaluation bar: a tall bar next to the board. The white part grows when White is better.
     *
     * The bar gets a target from the screen and moves towards it a little in every frame, so the
     * jumps of the engine do not make it flicker. It is drawn in two steps: the bar itself with SFML,
     * and the score as text with Dear ImGui on top of it.
     */
    class EvalBar
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates a bar at the middle position.
         *
         * @param t_origin The window position of the top left corner of the bar.
         * @param t_size The width and the height of the bar in pixels.
         */
        EvalBar(sf::Vector2f t_origin, sf::Vector2f t_size);

        EvalBar(const EvalBar& t_other) = delete;
        EvalBar(EvalBar&& t_other) noexcept = delete;
        EvalBar& operator=(const EvalBar& t_other) = delete;
        EvalBar& operator=(EvalBar&& t_other) noexcept = delete;

        ~EvalBar() noexcept = default;

        //-------------------------------------------------
        // State
        //-------------------------------------------------

        /**
         * @brief Sets the value the bar moves towards.
         *
         * @param t_whiteShare The part of the bar that belongs to White, from 0.0 to 1.0.
         * @param t_label The text shown on the bar, e.g. "+0.35". Empty for no text.
         */
        void SetTarget(double t_whiteShare, std::string t_label);

        /**
         * @brief Moves the shown value towards the target.
         *
         * @param t_dt The time since the previous frame.
         */
        void Update(sf::Time t_dt);

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Draws the bar with SFML.
         *
         * @param t_target The surface to draw on.
         * @param t_flipped True if the board is turned around: White is at the top then, and so is its part of the bar.
         */
        void Draw(sf::RenderTarget& t_target, bool t_flipped) const;

        /**
         * @brief Draws the score as text on the bar with Dear ImGui. Call it while a GUI frame is being built.
         *
         * @param t_flipped True if the board is turned around.
         */
        void DrawLabel(bool t_flipped) const;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        sf::Vector2f m_origin; ///< Top left corner of the bar in the window
        sf::Vector2f m_size; ///< Width and height of the bar in pixels
        double m_shown{ 0.5 }; ///< The part of White that is drawn right now
        double m_target{ 0.5 }; ///< The part of White the bar moves towards
        std::string m_label; ///< The text on the bar
    };
} // namespace sg::chess::ui
