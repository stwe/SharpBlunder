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
#include <algorithm>
#include <utility>
#include "core/I18n.h"
#include "ui/Layout.h"
#include "ui/screens/SettingsScreen.h"

namespace
{
    // The same window as the other pages: everything to the right of the navigation bar, without a title bar.
    constexpr ImGuiWindowFlags WINDOW_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                             ImGuiWindowFlags_NoResize |
                                             ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoSavedSettings };

    constexpr float LABEL_WIDTH{ 160.0f };
    constexpr float FIELD_WIDTH{ 280.0f };
    constexpr float MARGIN{ 24.0f };

    constexpr ImVec4 ERROR_COLOR{ 1.0f, 0.4f, 0.4f, 1.0f };

    // The label of a row: the text on the left, and the field starts at LABEL_WIDTH.
    void Label(const char* t_text)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(t_text);
        ImGui::SameLine(LABEL_WIDTH);
    }
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::SettingsScreen::SettingsScreen(core::Settings& t_settings, std::vector<core::Language> t_languages,
                                              ApplyFunction t_apply)
    : m_settings(t_settings)
    , m_languages(std::move(t_languages))
    , m_apply(std::move(t_apply))
{
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::SettingsScreen::Title() const noexcept
{
    return core::tr("Settings");
}

void sg::chess::ui::SettingsScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    // The page fills everything to the right of the navigation bar.
    ImGui::SetNextWindowPos({ NAVIGATION_WIDTH, 0.0f });
    ImGui::SetNextWindowSize({ displaySize.x - NAVIGATION_WIDTH, displaySize.y });

    // The padding is read when the window begins. It is taken back at once, or the popup of the list has it, too.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { MARGIN, MARGIN });
    const bool visible{ ImGui::Begin("##content", nullptr, WINDOW_FLAGS) };
    ImGui::PopStyleVar();

    if (visible)
    {
        ImGui::SeparatorText(core::tr("Settings"));
        ImGui::Spacing();

        ImGui::SeparatorText(core::tr("General"));
        DrawLanguage();

        ImGui::Spacing();
        ImGui::SeparatorText(core::tr("Analysis"));

        Label(core::tr("Engine"));
        if (ImGui::Checkbox(core::tr("Switch the engine on when the program starts"),
                            &m_settings.analysis.engineOnAtStart))
        {
            Changed();
        }

        ImGui::SetCursorPosX(LABEL_WIDTH + MARGIN);
        ImGui::TextDisabled("%s", core::tr("This takes effect the next time the program starts."));

        if (!m_error.empty())
        {
            ImGui::Spacing();
            ImGui::TextColored(ERROR_COLOR, "%s",
                               core::tr_format("The settings could not be saved: {}", m_error).c_str());
        }
    }
    ImGui::End();
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

void sg::chess::ui::SettingsScreen::DrawLanguage()
{
    // What "auto" comes to on this system, so that the user sees what the choice means.
    const std::string resolved{ core::resolve_language(core::Settings::AUTO_LANGUAGE, core::system_language(),
                                                       m_languages) };
    const auto nameOf = [&](const std::string& t_code)
    {
        const auto it{ std::ranges::find(m_languages, t_code, &core::Language::code) };
        return it == m_languages.end() ? t_code : it->nativeName;
    };

    const std::string automatic{ core::tr_format("Automatic ({})", nameOf(resolved)) };
    const bool isAutomatic{ m_settings.language == core::Settings::AUTO_LANGUAGE };

    Label(core::tr("Language"));
    ImGui::SetNextItemWidth(FIELD_WIDTH);

    if (ImGui::BeginCombo("##language", isAutomatic ? automatic.c_str() : nameOf(m_settings.language).c_str()))
    {
        if (ImGui::Selectable(automatic.c_str(), isAutomatic))
        {
            m_settings.language = core::Settings::AUTO_LANGUAGE;
            Changed();
        }

        for (const core::Language& language : m_languages)
        {
            if (ImGui::Selectable(language.nativeName.c_str(), m_settings.language == language.code))
            {
                m_settings.language = language.code;
                Changed();
            }
        }

        ImGui::EndCombo();
    }
}

void sg::chess::ui::SettingsScreen::Changed()
{
    m_error = m_apply ? m_apply().value_or("") : "";
}
