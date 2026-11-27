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
#include "ui/Layout.h"
#include "ui/screens/PlaceholderScreen.h"

namespace
{
    constexpr ImGuiWindowFlags WINDOW_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoSavedSettings };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::PlaceholderScreen::PlaceholderScreen(std::string t_title, std::string t_description)
    : m_title(std::move(t_title))
    , m_description(std::move(t_description))
{
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::PlaceholderScreen::Title() const noexcept
{
    return core::tr(m_title.c_str());
}

void sg::chess::ui::PlaceholderScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    // The page stands a little off the edges of the window, like the side panel of the analysis page.
    ImGui::SetNextWindowPos({ NAVIGATION_WIDTH + PAGE_MARGIN_SIDE, PAGE_MARGIN_TOP });
    ImGui::SetNextWindowSize({ displaySize.x - NAVIGATION_WIDTH - 2.0f * PAGE_MARGIN_SIDE,
                               displaySize.y - PAGE_MARGIN_TOP - PAGE_MARGIN_BOTTOM });

    if (ImGui::Begin("##content", nullptr, WINDOW_FLAGS))
    {
        ImGui::SeparatorText(core::tr(m_title.c_str()));
        ImGui::TextWrapped("%s", core::tr(m_description.c_str()));
        ImGui::Spacing();
        ImGui::TextDisabled("%s", core::tr("Not implemented yet."));
    }
    ImGui::End();
}
