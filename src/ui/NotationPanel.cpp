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
#include "ui/NotationPanel.h"

namespace
{
    constexpr float NUMBER_COLUMN_WIDTH{ 56.0f };
    constexpr float BUTTON_WIDTH{ 64.0f };

    // The line below the table: a separator and a row of buttons.
    float NavigationHeight()
    {
        return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.0f + 1.0f;
    }
} // namespace

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

bool sg::chess::ui::NotationPanel::DrawNavigation(core::GameState& t_game)
{
    bool changed{ false };

    // Four buttons with the spacing between them, centered in the window.
    const float spacing{ ImGui::GetStyle().ItemSpacing.x };
    const float width{ 4.0f * BUTTON_WIDTH + 3.0f * spacing };
    const float indent{ (ImGui::GetContentRegionAvail().x - width) / 2.0f };
    if (indent > 0.0f)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
    }

    // Runs one navigation action.
    const auto navigate = [&t_game, &changed](const bool t_enabled, const char* t_label,
                                              void (core::GameState::*t_action)() noexcept)
    {
        ImGui::BeginDisabled(!t_enabled);
        if (ImGui::Button(t_label, ImVec2(BUTTON_WIDTH, 0.0f)))
        {
            (t_game.*t_action)();
            changed = true;
        }
        ImGui::EndDisabled();
    };

    navigate(t_game.CanGoBack(), "|<", &core::GameState::GoToStart);
    ImGui::SameLine();
    navigate(t_game.CanGoBack(), "<", &core::GameState::GoBack);
    ImGui::SameLine();
    navigate(t_game.CanGoForward(), ">", &core::GameState::GoForward);
    ImGui::SameLine();
    navigate(t_game.CanGoForward(), ">|", &core::GameState::GoToEnd);

    return changed;
}

bool sg::chess::ui::NotationPanel::DrawMoveList(core::GameState& t_game)
{
    constexpr ImGuiTableFlags tableFlags{ ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV };

    if (!ImGui::BeginTable("##moves", 3, tableFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y - NavigationHeight())))
    {
        return false;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("##number", ImGuiTableColumnFlags_WidthFixed, NUMBER_COLUMN_WIDTH);
    ImGui::TableSetupColumn("White", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Black", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    const auto entries{ t_game.Entries() };
    bool changed{ false };

    for (std::size_t ply{ 1 }; ply < entries.size(); ++ply)
    {
        const auto& before{ entries[ply - 1].info.position };
        const bool whiteMoved{ before.SideToMove() == core::Color::WHITE };

        // A new row starts with each White move, and with the first move if Black begins.
        if (whiteMoved || ply == 1)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled(whiteMoved ? "%d." : "%d...", before.FullmoveNumber());
        }

        ImGui::TableSetColumnIndex(whiteMoved ? 1 : 2);

        // The same move text can appear many times, so every entry gets its own id.
        ImGui::PushID(static_cast<int>(ply));

        const bool isCurrent{ ply == t_game.CurrentPly() };
        if (ImGui::Selectable(entries[ply].san.c_str(), isCurrent))
        {
            t_game.GoTo(ply);
            changed = true;
        }

        if (isCurrent && m_scrollToCurrent)
        {
            ImGui::SetScrollHereY(0.5f);
        }

        ImGui::PopID();
    }

    m_scrollToCurrent = false;

    ImGui::EndTable();

    return changed;
}
