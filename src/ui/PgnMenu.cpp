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
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include "core/I18n.h"
#include "core/PositionCheck.h"
#include "core/Util.h"
#include "ui/PgnMenu.h"

namespace
{
    constexpr float BUTTON_WIDTH{ 76.0f };
    constexpr double NOTICE_SECONDS{ 2.5 };

    constexpr const char* MENU_POPUP{ "##pgn_menu" };
    constexpr const char* SAVE_POPUP{ "###save_pgn" };
    constexpr const char* OPEN_POPUP{ "###open_pgn" };
    constexpr const char* FEN_POPUP{ "###fen" };
    constexpr const char* REPLACE_POPUP{ "###replace_game" };
    constexpr const char* ERROR_POPUP{ "###error" };

    constexpr float PATH_WIDTH{ 460.0f };
    constexpr std::size_t MAX_LISTED_FILES{ 10 };

    // Where games are saved and where the window for opening a game looks first.
    std::filesystem::path GamesDirectory()
    {
        return sg::chess::core::data_dir() / "games";
    }

    // The files of the games directory with the PGN extension, the newest first.
    std::vector<std::filesystem::path> PgnFiles()
    {
        std::vector<std::filesystem::path> files;

        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(GamesDirectory(), ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".pgn")
            {
                files.push_back(entry.path());
            }
        }

        std::ranges::sort(files, [](const std::filesystem::path& t_a, const std::filesystem::path& t_b)
                          { return std::filesystem::last_write_time(t_a) > std::filesystem::last_write_time(t_b); });

        return files;
    }

    // The window is placed in the middle of the program window.
    void CenterNextWindow()
    {
        const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

        ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Appearing,
                                ImVec2(0.5f, 0.5f));
    }
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::PgnMenu::PgnMenu(core::RulesProvider& t_rules)
    : m_rules(t_rules)
{
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

float sg::chess::ui::PgnMenu::ButtonWidth()
{
    return BUTTON_WIDTH;
}

bool sg::chess::ui::PgnMenu::Draw(core::GameState& t_game, core::PgnHeaders& t_headers)
{
    bool opened{ false };

    // The button tells for a moment what has happened: "Copied", "Saved".
    const bool showNotice{ ImGui::GetTime() < m_noticeUntil };
    const std::string label{ showNotice ? m_notice : std::string{ "PGN" } };

    if (ImGui::Button((label + "###pgn").c_str(), ImVec2(BUTTON_WIDTH, 0.0f)))
    {
        ImGui::OpenPopup(MENU_POPUP);
    }

    // The menu only chooses. What follows is done below, outside of the menu, so the windows are opened from
    // the same place as the button.
    Action action{ Action::NONE };

    if (ImGui::BeginPopup(MENU_POPUP))
    {
        if (ImGui::MenuItem(core::tr("Copy PGN to clipboard")))
        {
            action = Action::COPY;
        }

        if (ImGui::MenuItem(core::tr("Paste PGN from clipboard")))
        {
            action = Action::PASTE;
        }

        ImGui::Separator();

        if (ImGui::MenuItem(core::tr("Save to file...")))
        {
            action = Action::SAVE;
        }

        if (ImGui::MenuItem(core::tr("Open file...")))
        {
            action = Action::OPEN;
        }

        ImGui::Separator();

        if (ImGui::MenuItem(core::tr("Copy FEN of this position")))
        {
            action = Action::COPY_FEN;
        }

        if (ImGui::MenuItem(core::tr("Set up from FEN...")))
        {
            action = Action::SETUP_FEN;
        }

        ImGui::EndPopup();
    }

    switch (action)
    {
    case Action::COPY:
        ImGui::SetClipboardText(core::pgn_write(t_game, t_headers).c_str());
        m_notice = core::tr("Copied");
        m_noticeUntil = ImGui::GetTime() + NOTICE_SECONDS;
        break;
    case Action::PASTE:
    {
        const char* text{ ImGui::GetClipboardText() };
        opened = Load(text != nullptr ? text : "", t_game, t_headers);
        break;
    }
    case Action::SAVE:
        SetPath((GamesDirectory() / ("analysis-" + core::format_local_time("%Y%m%d-%H%M%S") + ".pgn")).string());
        ImGui::OpenPopup(SAVE_POPUP);
        break;
    case Action::OPEN:
        SetPath(GamesDirectory().string() + "/");
        ImGui::OpenPopup(OPEN_POPUP);
        break;
    case Action::COPY_FEN:
        ImGui::SetClipboardText(t_game.Current().info.position.ToFen().c_str());
        m_notice = core::tr("Copied");
        m_noticeUntil = ImGui::GetTime() + NOTICE_SECONDS;
        break;
    case Action::SETUP_FEN:
        SetFen(t_game.Current().info.position.ToFen());
        ImGui::OpenPopup(FEN_POPUP);
        break;
    case Action::NONE:
        break;
    }

    // Windows that were asked for while another window was being built.
    if (m_wantReplacePopup)
    {
        ImGui::OpenPopup(REPLACE_POPUP);
        m_wantReplacePopup = false;
    }

    if (m_wantErrorPopup)
    {
        ImGui::OpenPopup(ERROR_POPUP);
        m_wantErrorPopup = false;
    }

    // The window for saving.
    CenterNextWindow();
    if (ImGui::BeginPopupModal(core::tr_id("Save PGN file", SAVE_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(core::tr("Save the game, with its variations and comments, as a PGN file:"));
        ImGui::SetNextItemWidth(PATH_WIDTH);
        ImGui::InputText("##path", m_path.data(), m_path.size());
        ImGui::Spacing();

        if (ImGui::Button(core::tr("Save")))
        {
            if (SaveFile(t_game, t_headers))
            {
                m_notice = core::tr("Saved");
                m_noticeUntil = ImGui::GetTime() + NOTICE_SECONDS;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // The window for opening: the path, and the newest files of the games directory to pick from.
    CenterNextWindow();
    if (ImGui::BeginPopupModal(core::tr_id("Open PGN file", OPEN_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(core::tr("Open a PGN file:"));
        ImGui::SetNextItemWidth(PATH_WIDTH);
        ImGui::InputText("##path", m_path.data(), m_path.size());

        const std::vector<std::filesystem::path> files{ PgnFiles() };
        if (!files.empty())
        {
            ImGui::Spacing();
            ImGui::TextDisabled("%s", core::tr("Saved games"));

            for (std::size_t i{ 0 }; i < std::min(files.size(), MAX_LISTED_FILES); ++i)
            {
                // A click only picks the file. By default, a click on an entry would close the window.
                if (ImGui::Selectable(files[i].filename().string().c_str(), false,
                                      ImGuiSelectableFlags_NoAutoClosePopups))
                {
                    SetPath(files[i].string());
                }
            }
        }

        ImGui::Spacing();

        if (ImGui::Button(core::tr("Open")))
        {
            opened = OpenFile(t_game, t_headers);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // The window for the position: the FEN of the position on the board, which can be changed or replaced by another.
    CenterNextWindow();
    if (ImGui::BeginPopupModal(core::tr_id("Position (FEN)", FEN_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(
            core::tr("The position on the board as FEN. Write or paste another one to start a game from it:"));
        ImGui::SetNextItemWidth(PATH_WIDTH);

        // A click into the field selects all of it, so a pasted or typed FEN replaces the old one.
        ImGui::InputText("##fen", m_fen.data(), m_fen.size(), ImGuiInputTextFlags_AutoSelectAll);
        ImGui::Spacing();

        if (ImGui::Button(core::tr("Copy")))
        {
            ImGui::SetClipboardText(m_fen.data());
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Paste")))
        {
            const char* text{ ImGui::GetClipboardText() };
            SetFen(text != nullptr ? text : "");
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Set up")))
        {
            opened = LoadFen(m_fen.data(), t_game, t_headers);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // The question before a game with moves is replaced.
    CenterNextWindow();
    if (ImGui::BeginPopupModal(core::tr_id("Replace the game?", REPLACE_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(core::tr("The game on the board has moves. Replace it with the opened game?"));
        ImGui::Spacing();

        if (ImGui::Button(core::tr("Replace")))
        {
            opened = m_pendingIsFen ? ReplaceWithFen(m_pending, t_game, t_headers)
                                    : Replace(m_pending, t_game, t_headers);
            m_pending.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_pending.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // What went wrong.
    CenterNextWindow();
    if (ImGui::BeginPopupModal(core::tr_id("Error", ERROR_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushTextWrapPos(PATH_WIDTH);
        ImGui::TextUnformatted(m_error.c_str());
        ImGui::PopTextWrapPos();
        ImGui::Spacing();

        if (ImGui::Button(core::tr("OK")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return opened;
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

bool sg::chess::ui::PgnMenu::Load(const std::string& t_text, core::GameState& t_game, core::PgnHeaders& t_headers)
{
    if (!t_game.HasMoves())
    {
        return Replace(t_text, t_game, t_headers);
    }

    // The text is read when the user has answered. A broken text is an error at that moment.
    m_pending = t_text;
    m_pendingIsFen = false;
    m_wantReplacePopup = true;

    return false;
}

bool sg::chess::ui::PgnMenu::LoadFen(const std::string& t_fen, core::GameState& t_game, core::PgnHeaders& t_headers)
{
    if (!t_game.HasMoves())
    {
        return ReplaceWithFen(t_fen, t_game, t_headers);
    }

    m_pending = t_fen;
    m_pendingIsFen = true;
    m_wantReplacePopup = true;

    return false;
}

bool sg::chess::ui::PgnMenu::ReplaceWithFen(const std::string& t_fen, core::GameState& t_game,
                                            core::PgnHeaders& t_headers)
{
    try
    {
        // Spaces at the ends come from copying and are no part of the FEN.
        const std::size_t first{ t_fen.find_first_not_of(" \t\r\n") };
        const std::size_t last{ t_fen.find_last_not_of(" \t\r\n") };
        const std::string fen{ first == std::string::npos ? "" : t_fen.substr(first, last - first + 1) };

        const core::Position position{ core::Position::FromFen(fen) };

        if (const auto problem{ core::check_position(position) })
        {
            ShowError(*problem);
            return false;
        }

        core::GameState game{ m_rules, position };
        t_game.ReplaceWith(std::move(game));
        t_headers = core::PgnHeaders::MakeDefault(core::format_local_time("%Y.%m.%d"));

        return true;
    }
    catch (const std::exception& t_error)
    {
        ShowError(t_error.what());
    }

    return false;
}

bool sg::chess::ui::PgnMenu::Replace(const std::string& t_text, core::GameState& t_game, core::PgnHeaders& t_headers)
{
    try
    {
        // The game is built on its own. The one on the board is only replaced when this has worked.
        core::PgnGame loaded{ core::pgn_read(t_text, m_rules) };

        t_game.ReplaceWith(std::move(loaded.game));
        t_headers = std::move(loaded.headers);

        return true;
    }
    catch (const std::exception& t_error)
    {
        ShowError(t_error.what());
    }

    return false;
}

bool sg::chess::ui::PgnMenu::SaveFile(const core::GameState& t_game, const core::PgnHeaders& t_headers)
{
    const std::filesystem::path path{ m_path.data() };

    std::error_code ec;
    if (path.has_parent_path())
    {
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream file{ path };
    if (!file)
    {
        ShowError(core::tr_format("The file cannot be written: {}", path.string()));
        return false;
    }

    file << core::pgn_write(t_game, t_headers);
    file.flush();

    if (!file)
    {
        ShowError(core::tr_format("The file could not be written completely: {}", path.string()));
        return false;
    }

    return true;
}

bool sg::chess::ui::PgnMenu::OpenFile(core::GameState& t_game, core::PgnHeaders& t_headers)
{
    const std::filesystem::path path{ m_path.data() };

    std::ifstream file{ path };
    if (!file)
    {
        ShowError(core::tr_format("The file cannot be read: {}", path.string()));
        return false;
    }

    std::ostringstream text;
    text << file.rdbuf();

    return Load(text.str(), t_game, t_headers);
}

void sg::chess::ui::PgnMenu::SetPath(const std::string& t_path)
{
    const std::size_t length{ std::min(t_path.size(), m_path.size() - 1) };

    std::memcpy(m_path.data(), t_path.data(), length);
    m_path[length] = '\0';
}

void sg::chess::ui::PgnMenu::SetFen(const std::string& t_fen)
{
    const std::size_t length{ std::min(t_fen.size(), m_fen.size() - 1) };

    std::memcpy(m_fen.data(), t_fen.data(), length);
    m_fen[length] = '\0';
}

void sg::chess::ui::PgnMenu::ShowError(const std::string& t_message)
{
    m_error = t_message;
    m_wantErrorPopup = true;
}
