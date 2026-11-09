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
#include <string>
#include "ui/Layout.h"
#include "ui/NavigationBar.h"

namespace
{
    constexpr float ENTRY_HEIGHT{ 36.0f };

    constexpr ImGuiWindowFlags WINDOW_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoSavedSettings };
}

//-------------------------------------------------
// Drawing
//-------------------------------------------------

void sg::chess::ui::NavigationBar::DrawGui(ScreenManager& t_screens) const
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    ImGui::SetNextWindowPos({ 0.0f, 0.0f });
    ImGui::SetNextWindowSize({ NAVIGATION_WIDTH, displaySize.y });

    if (ImGui::Begin("##navigation", nullptr, WINDOW_FLAGS))
    {
        ImGui::TextDisabled("SharpBlunder");
        ImGui::Separator();

        // Center the label vertically inside the taller entries.
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

        const auto current{ t_screens.CurrentId() };
        for (const auto& entry : t_screens.Entries())
        {
            const std::string label{ entry.screen->Title() };

            if (ImGui::Selectable(label.c_str(), current == entry.id, ImGuiSelectableFlags_None, ImVec2(0.0f, ENTRY_HEIGHT)))
            {
                t_screens.SwitchTo(entry.id);
            }
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();
}
