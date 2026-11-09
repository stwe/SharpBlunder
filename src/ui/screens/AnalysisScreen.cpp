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
#include "ui/Layout.h"
#include "ui/screens/AnalysisScreen.h"

namespace
{
    constexpr float SQUARE_SIZE{ 80.0f };
    constexpr float MARGIN{ 30.0f };
    constexpr float BOARD_TOP{ 40.0f };

    // The board starts to the right of the navigation bar.
    constexpr sf::Vector2f BOARD_ORIGIN{ sg::chess::ui::NAVIGATION_WIDTH + MARGIN, BOARD_TOP };
    constexpr float BOARD_PIXELS{ static_cast<float>(sg::chess::core::BOARD_SIZE) * SQUARE_SIZE };

    // The side panel starts to the right of the board and is as tall as the board.
    constexpr float PANEL_LEFT{ BOARD_ORIGIN.x + BOARD_PIXELS + MARGIN };

    constexpr ImGuiWindowFlags PANEL_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoSavedSettings };

    constexpr float NUMBER_COLUMN_WIDTH{ 56.0f };
    constexpr float BUTTON_WIDTH{ 44.0f };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::AnalysisScreen::AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules)
    : m_game(t_rules)
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, t_pieces)
    , m_input(m_boardView)
{
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::AnalysisScreen::Title() const noexcept
{
    return "Analysis";
}

void sg::chess::ui::AnalysisScreen::HandleEvent(const sf::Event& t_event)
{
    if (const auto move{ m_input.HandleEvent(t_event, m_game.Current().info) })
    {
        if (m_game.Play(*move))
        {
            OnPositionChanged();
        }

        return;
    }

    if (const auto* key = t_event.getIf<sf::Event::KeyPressed>())
    {
        switch (key->code)
        {
        case sf::Keyboard::Key::Left:
            m_game.GoBack();
            OnPositionChanged();
            break;
        case sf::Keyboard::Key::Right:
            m_game.GoForward();
            OnPositionChanged();
            break;
        case sf::Keyboard::Key::Home:
            m_game.GoToStart();
            OnPositionChanged();
            break;
        case sf::Keyboard::Key::End:
            m_game.GoToEnd();
            OnPositionChanged();
            break;
        case sf::Keyboard::Key::F:
            m_boardView.Flip();
            break;
        default:
            break;
        }
    }
}

void sg::chess::ui::AnalysisScreen::Draw(sf::RenderTarget& t_target)
{
    m_boardView.Draw(t_target, m_game.Current().info.position, BuildOverlay());
}

void sg::chess::ui::AnalysisScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    ImGui::SetNextWindowPos({ PANEL_LEFT, BOARD_TOP });
    ImGui::SetNextWindowSize({ displaySize.x - PANEL_LEFT - MARGIN, BOARD_PIXELS });

    if (ImGui::Begin("##analysis", nullptr, PANEL_FLAGS))
    {
        const std::string status{ StatusText() };
        ImGui::TextUnformatted(status.c_str());
        ImGui::Spacing();

        DrawControls();
        ImGui::Spacing();

        DrawMoveList();
    }
    ImGui::End();
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

sg::chess::ui::BoardOverlay sg::chess::ui::AnalysisScreen::BuildOverlay() const
{
    const auto& entry{ m_game.Current() };
    const auto& position{ entry.info.position };

    BoardOverlay overlay;
    overlay.lastMove = entry.move;

    if (entry.info.inCheck)
    {
        overlay.checkedSquare = position.KingSquare(position.SideToMove());
    }

    overlay.selectedSquare = m_input.SelectedSquare();
    overlay.targetSquares = m_input.TargetSquares(entry.info);

    if (m_input.IsDragging() && overlay.selectedSquare)
    {
        if (const auto piece{ position.At(*overlay.selectedSquare) })
        {
            overlay.dragged = BoardOverlay::DraggedPiece{ *piece, m_input.CursorPosition() };
        }
    }

    if (m_input.IsPromotionPending())
    {
        overlay.promotionColor = position.SideToMove();
    }

    return overlay;
}

std::string sg::chess::ui::AnalysisScreen::StatusText() const
{
    const auto& info{ m_game.Current().info };
    const bool whiteToMove{ info.position.SideToMove() == core::Color::WHITE };

    if (info.IsCheckmate())
    {
        return std::string("Checkmate - ") + (whiteToMove ? "Black" : "White") + " wins";
    }

    if (info.IsStalemate())
    {
        return "Stalemate - draw";
    }

    std::string text{ whiteToMove ? "White to move" : "Black to move" };

    if (info.inCheck)
    {
        text += " (check)";
    }

    // 100 halfmoves without a capture or a pawn move: a player may claim a draw.
    if (info.position.HalfmoveClock() >= 100)
    {
        text += " - 50-move rule: a draw can be claimed";
    }

    return text;
}

void sg::chess::ui::AnalysisScreen::OnPositionChanged()
{
    m_input.Reset();
    m_scrollToCurrent = true;
}

void sg::chess::ui::AnalysisScreen::DrawControls()
{
    // Runs one navigation action and updates the screen.
    const auto navigate = [this](const bool t_enabled, const char* t_label, void (core::GameState::*t_action)() noexcept)
    {
        ImGui::BeginDisabled(!t_enabled);
        if (ImGui::Button(t_label, ImVec2(BUTTON_WIDTH, 0.0f)))
        {
            (m_game.*t_action)();
            OnPositionChanged();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
    };

    navigate(m_game.CanGoBack(), "|<", &core::GameState::GoToStart);
    navigate(m_game.CanGoBack(), "<", &core::GameState::GoBack);
    navigate(m_game.CanGoForward(), ">", &core::GameState::GoForward);
    navigate(m_game.CanGoForward(), ">|", &core::GameState::GoToEnd);

    ImGui::Dummy(ImVec2(10.0f, 0.0f));
    ImGui::SameLine();

    if (ImGui::Button("Flip"))
    {
        m_boardView.Flip();
    }

    ImGui::SameLine();

    if (ImGui::Button("New"))
    {
        m_game.Reset();
        OnPositionChanged();
    }
}

void sg::chess::ui::AnalysisScreen::DrawMoveList()
{
    constexpr ImGuiTableFlags tableFlags{ ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV };

    if (!ImGui::BeginTable("##moves", 3, tableFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("##number", ImGuiTableColumnFlags_WidthFixed, NUMBER_COLUMN_WIDTH);
    ImGui::TableSetupColumn("White", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Black", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    const auto entries{ m_game.Entries() };

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

        const bool isCurrent{ ply == m_game.CurrentPly() };
        if (ImGui::Selectable(entries[ply].san.c_str(), isCurrent))
        {
            m_game.GoTo(ply);
            OnPositionChanged();
        }

        if (isCurrent && m_scrollToCurrent)
        {
            ImGui::SetScrollHereY(0.5f);
        }

        ImGui::PopID();
    }

    m_scrollToCurrent = false;

    ImGui::EndTable();
}
