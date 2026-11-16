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
#include <string_view>
#include <utility>
#include "core/San.h"
#include "core/Util.h"
#include "ui/Icons.h"
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
    constexpr float EVAL_BAR_GAP{ 10.0f };
    constexpr sf::Vector2f EVAL_BAR_ORIGIN{ sg::chess::ui::NAVIGATION_WIDTH + MARGIN, BOARD_TOP };
    constexpr sf::Vector2f EVAL_BAR_SIZE{ EVAL_BAR_WIDTH, BOARD_PIXELS };

    // The board starts to the right of the evaluation bar.
    constexpr sf::Vector2f BOARD_ORIGIN{ EVAL_BAR_ORIGIN.x + EVAL_BAR_WIDTH + EVAL_BAR_GAP, BOARD_TOP };

    // The side panel starts to the right of the board and is as tall as the board.
    constexpr float PANEL_LEFT{ BOARD_ORIGIN.x + BOARD_PIXELS + MARGIN };

    constexpr ImGuiWindowFlags PANEL_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoSavedSettings };

    constexpr float NUMBER_COLUMN_WIDTH{ 56.0f };
    constexpr float BUTTON_WIDTH{ 44.0f };

    // The analysis: how often the lines are taken over, and how many moves of a line are written in SAN.
    // Every move needs a question to the rules engine, so both are kept small.
    constexpr float REFRESH_INTERVAL{ 0.5f };
    constexpr std::size_t MAX_VARIATION_PLIES{ 6 };
    constexpr std::size_t MAX_CACHED_VARIATIONS{ 2000 };

    // The rows of the recommendations: the move starts after the dot and the score. The piece icon is a little
    // smaller than a row.
    constexpr float SCORE_OFFSET{ 4.0f };
    constexpr float MOVE_OFFSET{ 92.0f };
    constexpr float PIECE_ICON_SIZE{ 28.0f };
    constexpr float PIECE_ICON_GAP{ 2.0f };

    // The letters of the pieces in SAN. Castling ("O-O") starts with another letter and keeps its text.
    constexpr std::string_view PIECE_LETTERS{ "KQRBN" };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::AnalysisScreen::AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules,
                                              engine::Analyzer& t_analyzer, const core::OpeningBook& t_openings)
    : m_game(t_rules)
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, t_pieces)
    , m_input(m_boardView)
    , m_pieces(t_pieces)
    , m_rules(t_rules)
    , m_analyzer(t_analyzer)
    , m_openings(t_openings)
    , m_evalBar(EVAL_BAR_ORIGIN, EVAL_BAR_SIZE)
{
    m_analyzer.SetPosition(m_game.Current().info.position);
    m_sinceRefresh = REFRESH_INTERVAL;
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
    m_analyzer.SetRunning(m_analysisOn);
}

void sg::chess::ui::AnalysisScreen::OnExit()
{
    m_analyzer.SetRunning(false);
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
    else if (!m_analysisOn || !m_analyzer.IsAlive())
    {
        m_evalBar.SetTarget(0.5, "");
    }
    else
    {
        m_sinceRefresh += t_dt.asSeconds();

        if (m_sinceRefresh >= REFRESH_INTERVAL)
        {
            RefreshAnalysis();
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

        DrawAnalysis();
        ImGui::Spacing();

        DrawMoveList();
    }
    ImGui::End();

    m_evalBar.DrawLabel(m_boardView.IsFlipped());
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
    const std::optional<core::Move>& arrow{ m_hoveredMove ? m_hoveredMove : m_bestMove };
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
    m_scrollToCurrent = true;

    // The lines belong to the old position. They are dropped at once, the new ones follow in a moment.
    m_display.clear();
    m_assessment.clear();
    m_openLines.clear();
    m_bestMove.reset();
    m_hoveredMove.reset();
    m_sinceRefresh = REFRESH_INTERVAL;
    m_analyzer.SetPosition(m_game.Current().info.position);
    UpdateOpening();
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

void sg::chess::ui::AnalysisScreen::DrawAnalysis()
{
    // Set again below if the mouse rests on a row.
    m_hoveredMove.reset();

    if (ImGui::Checkbox("Engine recommendations", &m_analysisOn))
    {
        m_analyzer.SetRunning(m_analysisOn);

        m_display.clear();
        m_assessment.clear();
        m_openLines.clear();
        m_bestMove.reset();
        m_sinceRefresh = REFRESH_INTERVAL;
    }

    if (!m_analysisOn)
    {
        return;
    }

    if (!m_analyzer.IsAlive())
    {
        ImGui::TextWrapped("The analysis engine is not available.");
        return;
    }

    if (m_display.empty())
    {
        // No lines yet, or the game is over and there is nothing to analyze.
        if (!m_game.Current().info.legalMoves.empty())
        {
            ImGui::TextDisabled("Thinking...");
        }

        return;
    }

    // The depth sits at the right end of the line with the check box.
    const std::string depth{ "Depth " + std::to_string(m_display.front().depth) };
    ImGui::SameLine(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(depth.c_str()).x);
    ImGui::TextDisabled("%s", depth.c_str());

    ImGui::TextUnformatted(m_assessment.c_str());
    ImGui::Spacing();

    const float rowHeight{ ImGui::GetFrameHeight() };
    const float rowWidth{ ImGui::GetContentRegionAvail().x - rowHeight - ImGui::GetStyle().ItemSpacing.x };
    std::optional<core::Move> clicked;

    for (std::size_t i{ 0 }; i < m_display.size(); ++i)
    {
        const DisplayLine& line{ m_display[i] };
        ImGui::PushID(static_cast<int>(i));

        // The whole row is one clickable area. Dot, score and move are drawn on top of it.
        // It is highlighted like a button, not in the strong color of a selected entry.
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        const ImVec2 min{ ImGui::GetCursorScreenPos() };
        if (ImGui::Selectable("##row", false, ImGuiSelectableFlags_None, ImVec2(rowWidth, rowHeight)))
        {
            clicked = line.move;
        }

        ImGui::PopStyleColor(2);

        const bool hovered{ ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) };
        if (ImGui::IsItemHovered())
        {
            m_hoveredMove = line.move;
        }

        const ImU32 textColor{ ImGui::GetColorU32(ImGuiCol_Text) };
        const float textY{ min.y + (rowHeight - ImGui::GetTextLineHeight()) / 2.0f };
        ImDrawList* draw{ ImGui::GetWindowDrawList() };

        draw_quality_dot(min.x, min.y, rowHeight, line.quality);
        draw->AddText(ImVec2(min.x + rowHeight + SCORE_OFFSET, textY), textColor, line.score.c_str());

        float moveX{ min.x + MOVE_OFFSET };
        if (line.figure)
        {
            draw_piece_icon(m_pieces.Get(*line.figure), moveX, min.y + (rowHeight - PIECE_ICON_SIZE) / 2.0f,
                            PIECE_ICON_SIZE);
            moveX += PIECE_ICON_SIZE + PIECE_ICON_GAP;
        }

        draw->AddText(ImVec2(moveX, textY), textColor, line.moveText.c_str());

        // Over the dot the tooltip explains the color, anywhere else it explains the score.
        if (hovered && ImGui::BeginTooltip())
        {
            const ImVec2 dotMax{ min.x + rowHeight, min.y + rowHeight };

            if (ImGui::IsMouseHoveringRect(min, dotMax))
            {
                ImGui::TextUnformatted(describe_quality(line.quality));
            }
            else
            {
                ImGui::TextUnformatted(line.description.c_str());
                ImGui::TextDisabled("Click to play this move");
            }

            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        const bool open{ m_openLines.contains(line.move.ToUci()) };
        if (magnifier_button(open, rowHeight, open ? "Hide the line" : "Show the whole line"))
        {
            // The state belongs to the move, not to the row: when two lines swap places, the open one stays open.
            if (open)
            {
                m_openLines.erase(line.move.ToUci());
            }
            else
            {
                m_openLines.insert(line.move.ToUci());
            }
        }

        if (open)
        {
            ImGui::Indent(rowHeight);
            ImGui::TextWrapped("%s", line.variation.c_str());
            ImGui::Unindent(rowHeight);
        }

        ImGui::PopID();
    }

    // Playing a move changes the position and with it m_display, so this must wait until the loop is done.
    if (clicked && m_game.Play(*clicked))
    {
        OnPositionChanged();
    }
}

void sg::chess::ui::AnalysisScreen::RefreshAnalysis()
{
    const std::vector<engine::AnalysisLine> lines{ m_analyzer.Lines() };
    if (lines.empty())
    {
        return;
    }

    m_sinceRefresh = 0.0f;

    // The scores of the analyzer are already seen from White, like the bar.
    const core::Score& best{ lines.front().score };
    m_evalBar.SetTarget(best.WhiteShare(), best.ToString());
    m_assessment = best.Describe();

    // How much weaker a line is than the best one, seen from the side to move.
    const bool whiteToMove{ m_game.Current().info.position.SideToMove() == core::Color::WHITE };
    const int sign{ whiteToMove ? 1 : -1 };
    const int bestValue{ best.Centipawns() };

    m_display.clear();
    for (const engine::AnalysisLine& line : lines)
    {
        const int loss{ sign * (bestValue - line.score.Centipawns()) };
        const SanLine san{ ToSan(line) };

        // A line whose first move is not legal here cannot be played. That should not happen.
        if (san.firstMove.empty())
        {
            continue;
        }

        // A piece (not a pawn) gets an icon, and its letter in the SAN is dropped: the icon and "f3", not "Nf3".
        std::optional<core::Piece> figure{ m_game.Current().info.position.At(line.pv.front().from) };
        std::string moveText{ san.firstMove };
        const bool hasLetter{ PIECE_LETTERS.find(moveText.front()) != std::string_view::npos };
        if (figure && figure->type != core::PieceType::PAWN && hasLetter)
        {
            moveText.erase(0, 1);
        }
        else
        {
            figure.reset();
        }

        m_display.push_back({ line.depth, line.score.ToString(), line.score.Describe(), line.pv.front(), figure,
                              moveText, core::classify_loss(loss), san.text });
    }

    m_bestMove = lines.front().pv.front();
}

sg::chess::ui::AnalysisScreen::SanLine sg::chess::ui::AnalysisScreen::ToSan(const engine::AnalysisLine& t_line)
{
    const std::size_t plies{ std::min(t_line.pv.size(), MAX_VARIATION_PLIES) };

    std::vector<std::string> uci;
    for (std::size_t i{ 0 }; i < plies; ++i)
    {
        uci.push_back(t_line.pv[i].ToUci());
    }

    const std::string key{ m_game.Current().info.position.ToFen() + '|' + core::join_moves(uci) };
    if (const auto it{ m_variations.find(key) }; it != m_variations.end())
    {
        return it->second;
    }

    SanLine result;
    core::PositionInfo current{ m_game.Current().info };

    for (std::size_t i{ 0 }; i < plies; ++i)
    {
        const core::Move& move{ t_line.pv[i] };
        if (!current.IsLegal(move))
        {
            break;
        }

        core::PositionInfo after{ m_rules.Play(current.position, move) };
        const bool whiteMoves{ current.position.SideToMove() == core::Color::WHITE };

        // White's moves get a number. Black's only do if the line starts with Black.
        if (whiteMoves)
        {
            result.text += std::to_string(current.position.FullmoveNumber()) + ". ";
        }
        else if (i == 0)
        {
            result.text += std::to_string(current.position.FullmoveNumber()) + "... ";
        }

        const std::string san{ core::to_san(current, move, after) };
        if (i == 0)
        {
            result.firstMove = san;
        }

        result.text += san + ' ';
        current = std::move(after);
    }

    if (!result.text.empty())
    {
        result.text.pop_back();
    }

    if (m_variations.size() >= MAX_CACHED_VARIATIONS)
    {
        m_variations.clear();
    }

    m_variations.emplace(key, result);

    return result;
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
