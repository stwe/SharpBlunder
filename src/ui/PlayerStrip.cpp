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

#include <algorithm>
#include <array>
#include "ui/Icons.h"
#include "ui/PlayerStrip.h"

namespace
{
    constexpr float TEXT_OFFSET{ 10.0f };
    constexpr float PIECE_SIZE{ 20.0f };
    constexpr float PIECE_STEP{ 12.0f };   // pieces of one kind overlap: the next one starts this far on
    constexpr float GROUP_GAP{ 8.0f };     // the space between two kinds
    constexpr float PILL_PADDING{ 4.0f };
    constexpr float PILL_ROUNDING{ 4.0f };
    constexpr ImU32 PILL_COLOR{ IM_COL32(214, 210, 200, 255) };

    // The order in which captured pieces are shown: the most valuable first.
    constexpr std::array<sg::chess::core::PieceType, 5> ORDER{ sg::chess::core::PieceType::QUEEN,
                                                               sg::chess::core::PieceType::ROOK,
                                                               sg::chess::core::PieceType::BISHOP,
                                                               sg::chess::core::PieceType::KNIGHT,
                                                               sg::chess::core::PieceType::PAWN };
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::PlayerStrip::PlayerStrip(const PieceSet& t_pieces)
    : m_pieces(t_pieces)
{
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::PlayerStrip::Draw(const ImVec2 t_origin, const float t_height, const core::Color t_color,
                                      const std::string& t_name, const std::string& t_rating,
                                      const core::CapturedPieces& t_captured, const int t_lead) const
{
    ImDrawList* draw{ ImGui::GetBackgroundDrawList() };

    // The picture fills the height of the strip, apart from a little space above and below.
    const float picture{ t_height - 2.0f * PILL_PADDING / 2.0f };
    draw_avatar(draw, t_origin.x, t_origin.y + (t_height - picture) / 2.0f, picture, t_color);

    const float textX{ t_origin.x + picture + TEXT_OFFSET };
    const float lineHeight{ ImGui::GetTextLineHeight() };

    // Name and rating in the upper half.
    draw->AddText(ImVec2(textX, t_origin.y + 1.0f), ImGui::GetColorU32(ImGuiCol_Text), t_name.c_str());

    if (!t_rating.empty())
    {
        const float nameWidth{ ImGui::CalcTextSize(t_name.c_str()).x };
        const std::string rating{ "(" + t_rating + ")" };

        draw->AddText(ImVec2(textX + nameWidth + 6.0f, t_origin.y + 1.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled),
                      rating.c_str());
    }

    // The captured pieces in the lower half, on a light bar so that black pieces can be seen as well.
    const core::Color capturedColor{ t_color == core::Color::WHITE ? core::Color::BLACK : core::Color::WHITE };
    const float rowY{ t_origin.y + 1.0f + lineHeight + 2.0f };

    float width{ 0.0f };
    for (const core::PieceType type : ORDER)
    {
        const int count{ t_captured.count[static_cast<std::size_t>(type)] };
        if (count > 0)
        {
            width += PIECE_SIZE + PIECE_STEP * static_cast<float>(count - 1) + GROUP_GAP;
        }
    }

    if (width > 0.0f)
    {
        width -= GROUP_GAP;

        draw->AddRectFilled(ImVec2(textX - PILL_PADDING, rowY - 1.0f),
                            ImVec2(textX + width + PILL_PADDING, rowY + PIECE_SIZE + 1.0f), PILL_COLOR, PILL_ROUNDING);

        float x{ textX };
        for (const core::PieceType type : ORDER)
        {
            const int count{ t_captured.count[static_cast<std::size_t>(type)] };

            for (int i{ 0 }; i < count; ++i)
            {
                draw_piece_image(draw, m_pieces.Get(core::Piece{ type, capturedColor }), x, rowY, PIECE_SIZE);
                x += PIECE_STEP;
            }

            if (count > 0)
            {
                x += PIECE_SIZE - PIECE_STEP + GROUP_GAP;
            }
        }
    }

    if (t_lead > 0)
    {
        const std::string lead{ "+" + std::to_string(t_lead) };
        const float leadX{ width > 0.0f ? textX + width + PILL_PADDING + 8.0f : textX };

        draw->AddText(ImVec2(leadX, rowY + (PIECE_SIZE - lineHeight) / 2.0f), ImGui::GetColorU32(ImGuiCol_TextDisabled),
                      lead.c_str());
    }
}
