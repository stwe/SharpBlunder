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

#include <cstring>
#include <imgui.h>
#include "ui/Icons.h"

namespace
{
    constexpr ImU32 GOOD_COLOR{ IM_COL32(76, 175, 80, 255) };
    constexpr ImU32 OKAY_COLOR{ IM_COL32(240, 180, 40, 255) };
    constexpr ImU32 WEAK_COLOR{ IM_COL32(220, 70, 60, 255) };

    constexpr float DOT_RADIUS{ 0.3f }; // as a part of the item size
    constexpr int CIRCLE_SEGMENTS{ 24 };

    constexpr float LENS_RADIUS{ 0.23f };
    constexpr float LENS_CENTER{ 0.42f };
    constexpr float HANDLE_LENGTH{ 0.3f };
    constexpr float LINE_THICKNESS{ 2.0f };
    constexpr float HOVER_RADIUS{ 0.5f };

    // Color of a quality.
    ImU32 ColorOf(const sg::chess::core::MoveQuality t_quality)
    {
        switch (t_quality)
        {
        case sg::chess::core::MoveQuality::GOOD:
            return GOOD_COLOR;
        case sg::chess::core::MoveQuality::OKAY:
            return OKAY_COLOR;
        case sg::chess::core::MoveQuality::WEAK:
            return WEAK_COLOR;
        }

        return GOOD_COLOR;
    }
} // namespace

//-------------------------------------------------
// Icons
//-------------------------------------------------

const char* sg::chess::ui::describe_quality(const core::MoveQuality t_quality)
{
    switch (t_quality)
    {
    case core::MoveQuality::GOOD:
        return "Good move: the best one or almost as good.";
    case core::MoveQuality::OKAY:
        return "Playable, but clearly weaker than the best move.";
    case core::MoveQuality::WEAK:
        return "Weak move: much worse than the best one.";
    }

    return "";
}

void sg::chess::ui::draw_quality_dot(const float t_x, const float t_y, const float t_size,
                                     const core::MoveQuality t_quality)
{
    const ImVec2 center{ t_x + t_size / 2.0f, t_y + t_size / 2.0f };

    ImGui::GetWindowDrawList()->AddCircleFilled(center, t_size * DOT_RADIUS, ColorOf(t_quality), CIRCLE_SEGMENTS);
}

void sg::chess::ui::draw_piece_icon(const sf::Texture& t_texture, const float t_x, const float t_y, const float t_size)
{
    // Dear ImGui wants the OpenGL name of the texture, packed into its own texture type (as ImGui-SFML does it).
    const unsigned int handle{ t_texture.getNativeHandle() };
    ImTextureID id{};
    std::memcpy(&id, &handle, sizeof handle);

    ImGui::GetWindowDrawList()->AddImage(id, ImVec2(t_x, t_y), ImVec2(t_x + t_size, t_y + t_size));
}

bool sg::chess::ui::magnifier_button(const bool t_active, const float t_size, const char* t_tooltip)
{
    const bool clicked{ ImGui::InvisibleButton("##magnifier", ImVec2(t_size, t_size)) };

    const ImVec2 min{ ImGui::GetItemRectMin() };
    const ImVec2 lens{ min.x + t_size * LENS_CENTER, min.y + t_size * LENS_CENTER };
    ImDrawList* draw{ ImGui::GetWindowDrawList() };

    if (ImGui::IsItemHovered())
    {
        draw->AddCircleFilled(ImVec2(min.x + t_size / 2.0f, min.y + t_size / 2.0f), t_size * HOVER_RADIUS,
                              ImGui::GetColorU32(ImGuiCol_ButtonHovered), CIRCLE_SEGMENTS);
    }

    const ImU32 color{ ImGui::GetColorU32(t_active ? ImGuiCol_CheckMark : ImGuiCol_Text) };

    // The glass is a ring, the handle a short line to the lower right.
    constexpr float DIAGONAL{ 0.70710678f };
    const float radius{ t_size * LENS_RADIUS };
    const float handleStart{ radius + 1.0f };
    const float handleEnd{ radius + t_size * HANDLE_LENGTH };

    draw->AddCircle(lens, radius, color, CIRCLE_SEGMENTS, LINE_THICKNESS);
    draw->AddLine(ImVec2(lens.x + handleStart * DIAGONAL, lens.y + handleStart * DIAGONAL),
                  ImVec2(lens.x + handleEnd * DIAGONAL, lens.y + handleEnd * DIAGONAL), color, LINE_THICKNESS);

    ImGui::SetItemTooltip("%s", t_tooltip);

    return clicked;
}
