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
#include "core/I18n.h"
#include "core/San.h"
#include "core/Util.h"
#include "ui/EnginePanel.h"
#include "ui/Icons.h"

namespace
{
    // The analysis: how often the lines are taken over, and how many moves of a line are written in SAN.
    // Every move needs a question to the rules engine, so both are kept small.
    constexpr float REFRESH_INTERVAL{ 0.5f };
    constexpr std::size_t MAX_VARIATION_PLIES{ 6 };
    constexpr std::size_t MAX_CACHED_VARIATIONS{ 2000 };

    // The rows of the recommendations: the move starts after the dot and the score. The piece icon is a little
    // smaller than a row.
    constexpr float SCORE_OFFSET{ 4.0f };
    constexpr float MOVE_OFFSET{ 92.0f };
    constexpr float PIECE_ICON_SIZE{ 22.0f };
    constexpr float PIECE_ICON_GAP{ 3.0f };

    // The same in the whole line that the magnifying glass opens: the icons are smaller there.
    constexpr float VARIATION_ICON_SIZE{ 20.0f };
    constexpr float VARIATION_LINE_HEIGHT{ 24.0f };

    // The letters of the pieces in SAN. Castling ("O-O") starts with another letter and keeps its text.
    constexpr std::string_view PIECE_LETTERS{ "KQRBN" };
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::EnginePanel::EnginePanel(const PieceSet& t_pieces, core::RulesProvider& t_rules,
                                        engine::Analyzer& t_analyzer)
    : m_pieces(t_pieces)
    , m_rules(t_rules)
    , m_analyzer(t_analyzer)
{
}

//-------------------------------------------------
// Control
//-------------------------------------------------

void sg::chess::ui::EnginePanel::OnEnter()
{
    m_analyzer.SetRunning(m_analysisOn);
}

void sg::chess::ui::EnginePanel::OnExit()
{
    m_analyzer.SetRunning(false);
}

void sg::chess::ui::EnginePanel::PositionChanged(const core::PositionInfo& t_info)
{
    // The lines belong to the old position. They are dropped at once, the new ones follow in a moment.
    // The score stays: the bar keeps its place until the first line of the new position arrives.
    m_display.clear();
    m_assessment.clear();
    m_openLines.clear();
    m_bestMove.reset();
    m_hoveredMove.reset();
    m_sinceRefresh = REFRESH_INTERVAL;

    m_analyzer.SetPosition(t_info.position);
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

void sg::chess::ui::EnginePanel::Update(const sf::Time t_dt, const core::PositionInfo& t_info)
{
    if (!IsActive())
    {
        return;
    }

    m_sinceRefresh += t_dt.asSeconds();

    if (m_sinceRefresh >= REFRESH_INTERVAL)
    {
        Refresh(t_info);
    }
}

std::optional<sg::chess::core::Move> sg::chess::ui::EnginePanel::Draw(const core::PositionInfo& t_info)
{
    const float top{ ImGui::GetCursorPosY() };
    const auto clicked{ DrawContent(t_info) };

    // Fill up to the minimum height. The spacing after the last item is already part of the cursor position.
    const float missing{ MinimumHeight() - (ImGui::GetCursorPosY() - top) - ImGui::GetStyle().ItemSpacing.y };
    if (missing > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, missing));
    }

    return clicked;
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

std::optional<sg::chess::core::Move> sg::chess::ui::EnginePanel::DrawContent(const core::PositionInfo& t_info)
{
    // Set again below if the mouse rests on a row.
    m_hoveredMove.reset();

    if (ImGui::Checkbox(core::tr("Engine"), &m_analysisOn))
    {
        m_analyzer.SetRunning(m_analysisOn);

        m_display.clear();
        m_assessment.clear();
        m_openLines.clear();
        m_bestMove.reset();
        m_bestScore.reset();
        m_sinceRefresh = REFRESH_INTERVAL;
    }

    if (!m_analysisOn)
    {
        return std::nullopt;
    }

    if (!m_analyzer.IsAlive())
    {
        ImGui::TextWrapped("%s", core::tr("The analysis engine is not available."));
        return std::nullopt;
    }

    if (m_display.empty())
    {
        // No lines yet, or the game is over and there is nothing to analyze.
        if (!t_info.legalMoves.empty())
        {
            ImGui::TextDisabled("%s", core::tr("Thinking..."));
        }

        return std::nullopt;
    }

    // The depth sits at the right end of the line with the check box.
    const std::string depth{ core::tr_format("Depth {}", m_display.front().depth) };
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

        // The first move of the line, with the icon of the piece if there is one.
        const SanMove& first{ line.moves.front() };
        float moveX{ min.x + MOVE_OFFSET };
        if (first.figure)
        {
            draw_piece_icon(m_pieces.Get(*first.figure), moveX, min.y + (rowHeight - PIECE_ICON_SIZE) / 2.0f,
                            PIECE_ICON_SIZE);
            moveX += PIECE_ICON_SIZE + PIECE_ICON_GAP;
        }

        draw->AddText(ImVec2(moveX, textY), textColor, first.text.c_str());

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
                ImGui::TextDisabled("%s", core::tr("Click to play this move"));
            }

            ImGui::EndTooltip();
        }

        ImGui::SameLine();
        const bool open{ m_openLines.contains(line.move.ToUci()) };
        if (magnifier_button(open, rowHeight, open ? core::tr("Hide the line") : core::tr("Show the whole line")))
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
            DrawVariation(line.moves);
            ImGui::Unindent(rowHeight);
        }

        ImGui::PopID();
    }

    // The caller plays the move. That changes the position and with it m_display, so it waits until the loop is done.
    return clicked;
}

float sg::chess::ui::EnginePanel::MinimumHeight() const
{
    const float spacing{ ImGui::GetStyle().ItemSpacing.y };

    // The check box, the line with the verdict, a small gap and one row for every line of the analysis.
    const float header{ ImGui::GetFrameHeight() + spacing + ImGui::GetTextLineHeight() + spacing + spacing };
    const float rows{ static_cast<float>(m_analyzer.LineCount()) * (ImGui::GetFrameHeight() + spacing) };

    return header + rows;
}

void sg::chess::ui::EnginePanel::Refresh(const core::PositionInfo& t_info)
{
    const std::vector<engine::AnalysisLine> lines{ m_analyzer.Lines() };
    if (lines.empty())
    {
        return;
    }

    m_sinceRefresh = 0.0f;

    // The scores of the analyzer are already seen from White, like the bar.
    const core::Score& best{ lines.front().score };
    m_bestScore = best;
    m_assessment = best.Describe();

    // How much weaker a line is than the best one, seen from the side to move.
    const bool whiteToMove{ t_info.position.SideToMove() == core::Color::WHITE };
    const int sign{ whiteToMove ? 1 : -1 };
    const int bestValue{ best.Centipawns() };

    m_display.clear();
    for (const engine::AnalysisLine& line : lines)
    {
        const int loss{ sign * (bestValue - line.score.Centipawns()) };
        const SanLine san{ ToSan(t_info, line) };

        // A line whose first move is not legal here cannot be played. That should not happen.
        if (san.moves.empty())
        {
            continue;
        }

        m_display.push_back({ line.depth, line.score.ToString(), line.score.Describe(), line.pv.front(),
                              core::classify_loss(loss), san.moves });
    }

    m_bestMove = lines.front().pv.front();
}

sg::chess::ui::EnginePanel::SanLine sg::chess::ui::EnginePanel::ToSan(const core::PositionInfo& t_info,
                                                                         const engine::AnalysisLine& t_line)
{
    const std::size_t plies{ std::min(t_line.pv.size(), MAX_VARIATION_PLIES) };

    std::vector<std::string> uci;
    for (std::size_t i{ 0 }; i < plies; ++i)
    {
        uci.push_back(t_line.pv[i].ToUci());
    }

    const std::string key{ t_info.position.ToFen() + '|' + core::join_moves(uci) };
    if (const auto it{ m_variations.find(key) }; it != m_variations.end())
    {
        return it->second;
    }

    SanLine result;
    core::PositionInfo current{ t_info };

    for (std::size_t i{ 0 }; i < plies; ++i)
    {
        const core::Move& move{ t_line.pv[i] };
        if (!current.IsLegal(move))
        {
            break;
        }

        core::PositionInfo after{ m_rules.Play(current.position, move) };
        const bool whiteMoves{ current.position.SideToMove() == core::Color::WHITE };
        const std::string san{ core::to_san(current, move, after) };

        SanMove entry;

        // White's moves get a number. Black's only do if the line starts with Black.
        if (whiteMoves)
        {
            entry.number = std::to_string(current.position.FullmoveNumber()) + ".";
        }
        else if (i == 0)
        {
            entry.number = std::to_string(current.position.FullmoveNumber()) + "...";
        }

        // A piece (not a pawn) gets an icon, and its letter in the SAN is dropped: the icon and "f3", not "Nf3".
        // Castling ("O-O") starts with another letter and keeps its text.
        entry.text = san;
        const auto piece{ current.position.At(move.from) };
        if (piece && piece->type != core::PieceType::PAWN && PIECE_LETTERS.find(san.front()) != std::string_view::npos)
        {
            entry.figure = piece;
            entry.text.erase(0, 1);
        }

        result.moves.push_back(std::move(entry));
        current = std::move(after);
    }

    if (m_variations.size() >= MAX_CACHED_VARIATIONS)
    {
        m_variations.clear();
    }

    m_variations.emplace(key, result);

    return result;
}

void sg::chess::ui::EnginePanel::DrawVariation(const std::vector<SanMove>& t_moves) const
{
    const ImVec2 start{ ImGui::GetCursorScreenPos() };
    const float width{ ImGui::GetContentRegionAvail().x };
    const float gap{ ImGui::GetStyle().ItemSpacing.x };
    const float numberGap{ ImGui::CalcTextSize(" ").x };

    ImDrawList* draw{ ImGui::GetWindowDrawList() };
    const ImU32 textColor{ ImGui::GetColorU32(ImGuiCol_Text) };
    const ImU32 numberColor{ ImGui::GetColorU32(ImGuiCol_TextDisabled) };
    const float textOffset{ (VARIATION_LINE_HEIGHT - ImGui::GetTextLineHeight()) / 2.0f };

    float x{ 0.0f };
    float y{ 0.0f };

    for (const SanMove& move : t_moves)
    {
        const float numberWidth{ move.number.empty() ? 0.0f : ImGui::CalcTextSize(move.number.c_str()).x + numberGap };
        const float iconWidth{ move.figure ? VARIATION_ICON_SIZE + PIECE_ICON_GAP : 0.0f };
        const float moveWidth{ numberWidth + iconWidth + ImGui::CalcTextSize(move.text.c_str()).x };

        // A move is never split: it goes to the next line if it does not fit.
        if (x > 0.0f && x + moveWidth > width)
        {
            x = 0.0f;
            y += VARIATION_LINE_HEIGHT;
        }

        float cursor{ start.x + x };

        if (!move.number.empty())
        {
            draw->AddText(ImVec2(cursor, start.y + y + textOffset), numberColor, move.number.c_str());
            cursor += numberWidth;
        }

        if (move.figure)
        {
            const float iconTop{ start.y + y + (VARIATION_LINE_HEIGHT - VARIATION_ICON_SIZE) / 2.0f };
            draw_piece_icon(m_pieces.Get(*move.figure), cursor, iconTop, VARIATION_ICON_SIZE);
            cursor += iconWidth;
        }

        draw->AddText(ImVec2(cursor, start.y + y + textOffset), textColor, move.text.c_str());

        x += moveWidth + gap;
    }

    // The drawing above does not move the cursor, so the space it uses is reserved here.
    ImGui::Dummy(ImVec2(width, y + VARIATION_LINE_HEIGHT));
}
