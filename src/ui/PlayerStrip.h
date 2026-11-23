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

#include <imgui.h>
#include <string>
#include "core/Material.h"
#include "core/Piece.h"
#include "ui/PieceSet.h"

namespace sg::chess::ui
{
    //-------------------------------------------------
    // PlayerStrip
    //-------------------------------------------------

    /**
     * @brief The strip above or below the board that shows a player: a picture, the name and rating, and under them
     * the pieces that the player has captured, with the points by which the player is ahead.
     *
     * It is drawn behind the Dear ImGui windows, so it needs no window of its own. Call Draw() while a GUI frame is
     * being built.
     */
    class PlayerStrip
    {
    public:
        //-------------------------------------------------
        // Ctors. / Dtor.
        //-------------------------------------------------

        /**
         * @brief Creates the strip.
         *
         * @param t_pieces The piece images for the captured pieces. Not owned: they must outlive this strip.
         */
        explicit PlayerStrip(const PieceSet& t_pieces);

        PlayerStrip(const PlayerStrip& t_other) = delete;
        PlayerStrip(PlayerStrip&& t_other) noexcept = delete;
        PlayerStrip& operator=(const PlayerStrip& t_other) = delete;
        PlayerStrip& operator=(PlayerStrip&& t_other) noexcept = delete;

        ~PlayerStrip() noexcept = default;

        //-------------------------------------------------
        // Drawing
        //-------------------------------------------------

        /**
         * @brief Draws the strip.
         *
         * @param t_origin The window position of the top left corner.
         * @param t_height The height of the strip in pixels.
         * @param t_color The side of the player.
         * @param t_name The name of the player.
         * @param t_rating The rating, or an empty text if it is not known.
         * @param t_captured The pieces that the player has captured.
         * @param t_lead The points by which the player is ahead in material. Zero or less shows nothing.
         */
        void Draw(ImVec2 t_origin, float t_height, core::Color t_color, const std::string& t_name,
                  const std::string& t_rating, const core::CapturedPieces& t_captured, int t_lead) const;

    private:
        //-------------------------------------------------
        // Member
        //-------------------------------------------------

        const PieceSet& m_pieces;  ///< The piece images (not owned)
    };
}
