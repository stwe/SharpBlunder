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

#include <imgui.h>
#include <utility>
#include "core/I18n.h"
#include "ui/TutorPanel.h"

namespace
{
    // The look of the box: a surface that is a little lighter than the panel, and a stripe at the left.
    constexpr ImVec4 BOX_COLOR{ 0.15f, 0.17f, 0.21f, 1.0f };
    constexpr ImU32 STRIPE_COLOR{ IM_COL32(86, 148, 214, 255) };
    constexpr float STRIPE_WIDTH{ 3.0f };
    constexpr float ROUNDING{ 4.0f };
    constexpr ImVec2 PADDING{ 12.0f, 6.0f };
} // namespace

//-------------------------------------------------
// Text
//-------------------------------------------------

void sg::chess::ui::TutorPanel::SetText(std::string t_text)
{
    m_text = std::move(t_text);
}

bool sg::chess::ui::TutorPanel::HasText() const noexcept
{
    return !m_text.empty();
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::TutorPanel::Draw() const
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, BOX_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, ROUNDING);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, PADDING);

    if (ImGui::BeginChild("##tutor", ImVec2(0.0f, HEIGHT), ImGuiChildFlags_AlwaysUseWindowPadding))
    {
        // The stripe is drawn over the left edge of the box.
        const ImVec2 corner{ ImGui::GetWindowPos() };
        const ImVec2 size{ ImGui::GetWindowSize() };
        ImGui::GetWindowDrawList()->AddRectFilled(corner, ImVec2(corner.x + STRIPE_WIDTH, corner.y + size.y),
                                                  STRIPE_COLOR, ROUNDING, ImDrawFlags_RoundCornersLeft);

        ImGui::TextDisabled("%s", core::tr("Tutor"));

        if (m_text.empty())
        {
            // TextDisabled() does not wrap, and the German hint is longer than the box is wide.
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", core::tr("There is no comment yet. The tutor will write here."));
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextWrapped("%s", m_text.c_str());
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}
