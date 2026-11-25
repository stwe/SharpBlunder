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
#include "core/I18n.h"
#include "ui/Icons.h"

namespace
{
    constexpr ImU32 GOOD_COLOR{ IM_COL32(76, 175, 80, 255) };
    constexpr ImU32 OKAY_COLOR{ IM_COL32(240, 180, 40, 255) };
    constexpr ImU32 WEAK_COLOR{ IM_COL32(220, 70, 60, 255) };

    // The light square the piece icons stand on, like the light squares of the board. Without it, black pieces
    // would hardly be seen on the dark panel.
    constexpr ImU32 PIECE_BACKGROUND{ IM_COL32(240, 217, 181, 255) };
    constexpr float PIECE_BACKGROUND_ROUNDING{ 4.0f };

    constexpr float DOT_RADIUS{ 0.3f }; // as a part of the item size
    constexpr int CIRCLE_SEGMENTS{ 24 };

    constexpr float LENS_RADIUS{ 0.23f };
    constexpr float LENS_CENTER{ 0.42f };
    constexpr float HANDLE_LENGTH{ 0.3f };
    constexpr float LINE_THICKNESS{ 2.0f };
    constexpr float HOVER_RADIUS{ 0.5f };

    // The speech bubble, as parts of the item size: the rounded rectangle and the tail at its lower left.
    constexpr float BUBBLE_LEFT{ 0.1f };
    constexpr float BUBBLE_RIGHT{ 0.9f };
    constexpr float BUBBLE_TOP{ 0.14f };
    constexpr float BUBBLE_BOTTOM{ 0.66f };
    constexpr float BUBBLE_ROUNDING{ 0.2f };
    constexpr float TAIL_FROM{ 0.2f };
    constexpr float TAIL_TO{ 0.4f };
    constexpr float TAIL_TIP{ 0.9f };
    constexpr float DOT_SIZE{ 0.055f };

    // The picture of a player, as parts of its size.
    constexpr float AVATAR_ROUNDING{ 0.12f };
    constexpr float AVATAR_HEAD_Y{ 0.36f };
    constexpr float AVATAR_HEAD_RADIUS{ 0.18f };
    constexpr float AVATAR_BODY_Y{ 1.02f };
    constexpr float AVATAR_BODY_RADIUS{ 0.36f };

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
        return core::tr("Good move: the best one or almost as good.");
    case core::MoveQuality::OKAY:
        return core::tr("Playable, but clearly weaker than the best move.");
    case core::MoveQuality::WEAK:
        return core::tr("Weak move: much worse than the best one.");
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

    ImDrawList* draw{ ImGui::GetWindowDrawList() };
    const ImVec2 min{ t_x, t_y };
    const ImVec2 max{ t_x + t_size, t_y + t_size };

    draw->AddRectFilled(min, max, PIECE_BACKGROUND, PIECE_BACKGROUND_ROUNDING);
    draw->AddImage(id, min, max);
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

bool sg::chess::ui::comment_icon(const bool t_filled, const bool t_active, const float t_size,
                                 const unsigned int t_color)
{
    const bool clicked{ ImGui::InvisibleButton("##comment", ImVec2(t_size, t_size)) };
    const bool hovered{ ImGui::IsItemHovered() };

    const ImVec2 min{ ImGui::GetItemRectMin() };
    ImDrawList* draw{ ImGui::GetWindowDrawList() };

    const ImVec2 topLeft{ min.x + t_size * BUBBLE_LEFT, min.y + t_size * BUBBLE_TOP };
    const ImVec2 bottomRight{ min.x + t_size * BUBBLE_RIGHT, min.y + t_size * BUBBLE_BOTTOM };
    const float rounding{ t_size * BUBBLE_ROUNDING };

    // The tail points down to the left, like the one of a speech bubble in a comic.
    const ImVec2 tailA{ min.x + t_size * TAIL_FROM, bottomRight.y - 1.0f };
    const ImVec2 tailB{ min.x + t_size * TAIL_TO, bottomRight.y - 1.0f };
    const ImVec2 tailTip{ min.x + t_size * TAIL_FROM, min.y + t_size * TAIL_TIP };

    if (t_filled)
    {
        // A little brighter under the mouse.
        const ImU32 color{ hovered ? IM_COL32(255, 255, 255, 255) : static_cast<ImU32>(t_color) };

        draw->AddRectFilled(topLeft, bottomRight, color, rounding);
        draw->AddTriangleFilled(tailA, tailB, tailTip, color);

        // Three dots, as if there was text in the bubble.
        const float y{ (topLeft.y + bottomRight.y) / 2.0f };
        const float step{ (bottomRight.x - topLeft.x) / 4.0f };
        for (int i{ 1 }; i <= 3; ++i)
        {
            draw->AddCircleFilled(ImVec2(topLeft.x + step * static_cast<float>(i), y), t_size * DOT_SIZE,
                                  IM_COL32(40, 40, 40, 255), 8);
        }

        if (t_active)
        {
            const ImVec2 outerTopLeft{ topLeft.x - 2.0f, topLeft.y - 2.0f };
            const ImVec2 outerBottomRight{ bottomRight.x + 2.0f, bottomRight.y + 2.0f };

            draw->AddRect(outerTopLeft, outerBottomRight, ImGui::GetColorU32(ImGuiCol_Text), rounding + 2.0f, 0, 1.0f);
        }
    }
    else
    {
        // Only the outline, faint: there is nothing to read yet.
        const ImU32 color{ ImGui::GetColorU32(hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled) };

        draw->AddRect(topLeft, bottomRight, color, rounding, 0, 1.2f);
        draw->AddTriangle(tailA, tailB, tailTip, color, 1.2f);
    }

    return clicked;
}

bool sg::chess::ui::pencil_button(const float t_size, const char* t_tooltip)
{
    const bool clicked{ ImGui::InvisibleButton("##pencil", ImVec2(t_size, t_size)) };
    const bool hovered{ ImGui::IsItemHovered() };

    const ImVec2 min{ ImGui::GetItemRectMin() };
    ImDrawList* draw{ ImGui::GetWindowDrawList() };

    if (hovered)
    {
        draw->AddCircleFilled(ImVec2(min.x + t_size / 2.0f, min.y + t_size / 2.0f), t_size * HOVER_RADIUS,
                              ImGui::GetColorU32(ImGuiCol_ButtonHovered), CIRCLE_SEGMENTS);
    }

    const ImU32 color{ ImGui::GetColorU32(ImGuiCol_Text) };

    // The pencil lies diagonally: the tip at the lower left, the end at the upper right. The body is a thin
    // quadrangle, the tip a triangle.
    const auto point = [&min, t_size](const float t_x, const float t_y)
    {
        return ImVec2(min.x + t_size * t_x, min.y + t_size * t_y);
    };

    draw->AddQuadFilled(point(0.30f, 0.62f), point(0.38f, 0.70f), point(0.82f, 0.26f), point(0.74f, 0.18f), color);
    draw->AddTriangleFilled(point(0.22f, 0.78f), point(0.30f, 0.62f), point(0.38f, 0.70f), color);
    draw->AddLine(point(0.20f, 0.80f), point(0.45f, 0.80f), color, 1.5f);

    ImGui::SetItemTooltip("%s", t_tooltip);

    return clicked;
}

void sg::chess::ui::draw_piece_image(ImDrawList* t_draw, const sf::Texture& t_texture, const float t_x,
                                     const float t_y, const float t_size)
{
    const unsigned int handle{ t_texture.getNativeHandle() };
    ImTextureID id{};
    std::memcpy(&id, &handle, sizeof handle);

    t_draw->AddImage(id, ImVec2(t_x, t_y), ImVec2(t_x + t_size, t_y + t_size));
}

void sg::chess::ui::draw_avatar(ImDrawList* t_draw, const float t_x, const float t_y, const float t_size,
                                const core::Color t_color)
{
    const bool white{ t_color == core::Color::WHITE };
    const ImU32 background{ white ? IM_COL32(232, 230, 224, 255) : IM_COL32(64, 62, 58, 255) };
    const ImU32 figure{ white ? IM_COL32(98, 96, 92, 255) : IM_COL32(196, 194, 188, 255) };

    const ImVec2 min{ t_x, t_y };
    const ImVec2 max{ t_x + t_size, t_y + t_size };

    t_draw->AddRectFilled(min, max, background, t_size * AVATAR_ROUNDING);

    // The head is a circle, the shoulders are a big circle below it. Everything outside of the square is cut off.
    t_draw->PushClipRect(min, max, true);
    t_draw->AddCircleFilled(ImVec2(t_x + t_size * 0.5f, t_y + t_size * AVATAR_HEAD_Y), t_size * AVATAR_HEAD_RADIUS,
                            figure, CIRCLE_SEGMENTS);
    t_draw->AddCircleFilled(ImVec2(t_x + t_size * 0.5f, t_y + t_size * AVATAR_BODY_Y), t_size * AVATAR_BODY_RADIUS,
                            figure, CIRCLE_SEGMENTS);
    t_draw->PopClipRect();
}
