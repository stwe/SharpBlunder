#include <imgui.h>
#include <utility>
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
    return m_title;
}

void sg::chess::ui::PlaceholderScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    // The page fills everything to the right of the navigation bar.
    ImGui::SetNextWindowPos({ NAVIGATION_WIDTH, 0.0f });
    ImGui::SetNextWindowSize({ displaySize.x - NAVIGATION_WIDTH, displaySize.y });

    if (ImGui::Begin("##content", nullptr, WINDOW_FLAGS))
    {
        ImGui::SeparatorText(m_title.c_str());
        ImGui::TextWrapped("%s", m_description.c_str());
        ImGui::Spacing();
        ImGui::TextDisabled("Not implemented yet.");
    }
    ImGui::End();
}
