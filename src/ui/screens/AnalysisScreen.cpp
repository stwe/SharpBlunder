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
#include <imgui.h>
#include "ui/Layout.h"
#include "ui/screens/AnalysisScreen.h"

namespace
{
    constexpr float SQUARE_SIZE{ 80.0f };
    constexpr float MARGIN{ 30.0f };
    constexpr float BOARD_TOP{ 40.0f };

    constexpr float BOARD_PIXELS{ static_cast<float>(sg::chess::core::BOARD_SIZE) * SQUARE_SIZE };

    // The evaluation bar starts to the right of the navigation bar and is as tall as the board.
    constexpr float EVAL_BAR_WIDTH{ 36.0f };
    // The gap also holds the numbers of the ranks. The files are written below the board, where there is room.
    constexpr float EVAL_BAR_GAP{ 26.0f };
    constexpr sf::Vector2f EVAL_BAR_ORIGIN{ sg::chess::ui::NAVIGATION_WIDTH + MARGIN, BOARD_TOP };
    constexpr sf::Vector2f EVAL_BAR_SIZE{ EVAL_BAR_WIDTH, BOARD_PIXELS };

    // The board starts to the right of the evaluation bar.
    constexpr sf::Vector2f BOARD_ORIGIN{ EVAL_BAR_ORIGIN.x + EVAL_BAR_WIDTH + EVAL_BAR_GAP, BOARD_TOP };

    // The side panel starts to the right of the board and is as tall as the board.
    // The gap to the board is smaller than the margin, so the panel stays wide enough for its buttons.
    constexpr float PANEL_GAP{ 18.0f };
    constexpr float PANEL_LEFT{ BOARD_ORIGIN.x + BOARD_PIXELS + PANEL_GAP };

    constexpr ImGuiWindowFlags PANEL_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoSavedSettings };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::AnalysisScreen::AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules,
                                              engine::Analyzer& t_analyzer, const core::OpeningBook& t_openings)
    : m_game(t_rules)
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, t_pieces)
    , m_input(m_boardView)
    , m_openings(t_openings)
    , m_evalBar(EVAL_BAR_ORIGIN, EVAL_BAR_SIZE)
    , m_enginePanel(t_pieces, t_rules, t_analyzer)
{
    m_enginePanel.PositionChanged(m_game.Current().info);
    UpdateOpening();
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::AnalysisScreen::Title() const noexcept
{
    return "Analysis";
}

void sg::chess::ui::AnalysisScreen::OnEnter()
{
    m_enginePanel.OnEnter();
}

void sg::chess::ui::AnalysisScreen::OnExit()
{
    m_enginePanel.OnExit();
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

void sg::chess::ui::AnalysisScreen::Update(const sf::Time t_dt)
{
    const auto& info{ m_game.Current().info };
    const bool whiteToMove{ info.position.SideToMove() == core::Color::WHITE };

    // A finished game needs no engine: the result is known.
    if (info.IsCheckmate())
    {
        m_evalBar.SetTarget(whiteToMove ? 0.0 : 1.0, whiteToMove ? "0-1" : "1-0");
    }
    else if (info.IsStalemate())
    {
        m_evalBar.SetTarget(0.5, "1/2");
    }
    else if (!m_enginePanel.IsActive())
    {
        m_evalBar.SetTarget(0.5, "");
    }
    else
    {
        m_enginePanel.Update(t_dt, info);

        // The scores of the analyzer are already seen from White, like the bar.
        if (const auto& score{ m_enginePanel.BestScore() })
        {
            m_evalBar.SetTarget(score->WhiteShare(), score->ToString());
        }
    }

    m_evalBar.Update(t_dt);
}

void sg::chess::ui::AnalysisScreen::Draw(sf::RenderTarget& t_target)
{
    m_evalBar.Draw(t_target, m_boardView.IsFlipped());
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

        if (m_opening)
        {
            ImGui::TextDisabled("%s", m_opening->eco.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s", m_opening->name.c_str());
        }

        ImGui::Spacing();

        DrawControls();
        ImGui::Spacing();

        if (const auto clicked{ m_enginePanel.Draw(m_game.Current().info) })
        {
            // Playing a move changes the position, so it is done after the panel has been drawn.
            if (m_game.Play(*clicked))
            {
                OnPositionChanged();
            }
        }

        ImGui::Spacing();

        if (m_notationPanel.DrawMoveList(m_game))
        {
            OnPositionChanged();
        }
    }
    ImGui::End();

    m_evalBar.DrawLabel(m_boardView.IsFlipped());
    m_boardView.DrawCoordinates();
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

    // The arrow shows the move of the row under the mouse, or else the best move.
    const std::optional<core::Move> arrow{ m_enginePanel.ArrowMove() };
    if (arrow && entry.info.IsLegal(*arrow))
    {
        overlay.bestMove = arrow;
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

void sg::chess::ui::AnalysisScreen::UpdateOpening()
{
    m_opening.reset();

    const auto entries{ m_game.Entries() };

    // Walk back until a position has a name. A game that has left the book keeps the last name.
    for (std::size_t ply{ m_game.CurrentPly() + 1 }; ply > 0; --ply)
    {
        if (const auto opening{ m_openings.Find(entries[ply - 1].info.position) })
        {
            m_opening = opening;
            return;
        }
    }
}

void sg::chess::ui::AnalysisScreen::OnPositionChanged()
{
    m_input.Reset();
    m_notationPanel.ScrollToCurrent();
    m_enginePanel.PositionChanged(m_game.Current().info);
    UpdateOpening();
}

void sg::chess::ui::AnalysisScreen::DrawControls()
{
    if (m_notationPanel.DrawNavigation(m_game))
    {
        OnPositionChanged();
    }

    ImGui::SameLine();
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
