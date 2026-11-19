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
#include <optional>
#include <string>
#include "ui/NotationPanel.h"

namespace
{
    using GameState = sg::chess::core::GameState;

    constexpr float NUMBER_COLUMN_WIDTH{ 56.0f };
    constexpr float BUTTON_WIDTH{ 64.0f };

    // A variation is indented, and the closing parenthesis of a nested one sits close to the last move.
    constexpr float VARIATION_INDENT{ 14.0f };
    constexpr float PARENTHESIS_SPACING{ 2.0f };

    constexpr ImGuiTableFlags TABLE_FLAGS{ ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                           ImGuiTableFlags_NoSavedSettings };

    // The line below the table: a separator and a row of buttons.
    float NavigationHeight()
    {
        return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.0f + 1.0f;
    }

    // What the user asked for while the list was drawn. It is carried out afterwards, because it changes the game
    // that is being drawn.
    struct Requests
    {
        std::optional<GameState::NodeId> select;   // go to this move
        std::optional<GameState::NodeId> promote;  // make this line the main line
        std::optional<GameState::NodeId> remove;   // delete this move and everything after it
    };

    struct Context
    {
        const GameState& game;
        bool scrollToCurrent;
        Requests requests;
    };

    // Variations are written like text: move after move, wrapped at the end of the line.
    struct Flow
    {
        float right{ 0.0f };     // x of the right edge, in window coordinates
        float lastMaxX{ 0.0f };  // x of the right end of the last item
        bool first{ true };      // nothing was placed yet
    };

    // Starts the next item of the flow: next to the last one if it fits, else in the next line.
    void Place(Flow& t_flow, const float t_width, const float t_spacing)
    {
        if (!t_flow.first && t_flow.lastMaxX + t_spacing + t_width <= t_flow.right)
        {
            ImGui::SameLine(0.0f, t_spacing);
        }

        t_flow.first = false;
    }

    // One move as a clickable item: a click goes there, the context menu changes the game.
    void MoveItem(Context& t_ctx, const GameState::NodeId t_id, const ImVec2 t_size, Flow* t_flow)
    {
        const GameState::Node& node{ t_ctx.game.At(t_id) };
        const bool isCurrent{ t_id == t_ctx.game.CurrentId() };

        // The same move text can appear many times, so every move gets its own id.
        ImGui::PushID(static_cast<int>(t_id));

        if (ImGui::Selectable(node.san.c_str(), isCurrent, ImGuiSelectableFlags_None, t_size))
        {
            t_ctx.requests.select = t_id;
        }

        if (t_flow != nullptr)
        {
            t_flow->lastMaxX = ImGui::GetItemRectMax().x;
        }

        if (isCurrent && t_ctx.scrollToCurrent)
        {
            ImGui::SetScrollHereY(0.5f);
        }

        if (ImGui::BeginPopupContextItem("##move"))
        {
            if (ImGui::MenuItem("Make main line", nullptr, false, !t_ctx.game.IsMainLine(t_id)))
            {
                t_ctx.requests.promote = t_id;
            }

            if (ImGui::MenuItem("Delete from here"))
            {
                t_ctx.requests.remove = t_id;
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // Writes text that cannot be clicked (a move number, a parenthesis) into the flow.
    void TextToken(Flow& t_flow, const std::string& t_text, const float t_spacing)
    {
        Place(t_flow, ImGui::CalcTextSize(t_text.c_str()).x, t_spacing);
        ImGui::TextDisabled("%s", t_text.c_str());
        t_flow.lastMaxX = ImGui::GetItemRectMax().x;
    }

    // Writes a line of moves into the flow, with move numbers. The alternatives to a move follow it in parentheses.
    // A nested line starts with an opening parenthesis and ends with a closing one.
    void EmitLine(Context& t_ctx, Flow& t_flow, const GameState::NodeId t_first, const bool t_nested)
    {
        const float spacing{ ImGui::GetStyle().ItemSpacing.x };

        std::optional<GameState::NodeId> id{ t_first };
        bool needNumber{ true };
        bool opened{ false };

        while (id)
        {
            const GameState::Node& node{ t_ctx.game.At(*id) };
            const GameState::Node& parent{ t_ctx.game.At(*node.parent) };
            const bool whiteMoves{ parent.info.position.SideToMove() == sg::chess::core::Color::WHITE };
            const std::string number{ std::to_string(parent.info.position.FullmoveNumber()) };

            // White's moves get a number. A Black move only does at the start of a line and after a variation.
            if (whiteMoves || needNumber)
            {
                const std::string text{ std::string(t_nested && !opened ? "(" : "") + number +
                                        (whiteMoves ? "." : "...") };
                TextToken(t_flow, text, spacing);
            }

            opened = true;
            needNumber = false;

            const float width{ ImGui::CalcTextSize(t_ctx.game.At(*id).san.c_str()).x };
            Place(t_flow, width, spacing);
            MoveItem(t_ctx, *id, ImVec2(width, 0.0f), &t_flow);

            // The other moves of this position are alternatives to this one, if this one is the main move.
            if (parent.children.size() > 1 && parent.children.front() == *id)
            {
                for (std::size_t i{ 1 }; i < parent.children.size(); ++i)
                {
                    EmitLine(t_ctx, t_flow, parent.children[i], true);
                }

                needNumber = true;
            }

            id = node.children.empty() ? std::nullopt : std::optional{ node.children.front() };
        }

        if (t_nested)
        {
            TextToken(t_flow, ")", PARENTHESIS_SPACING);
        }
    }

    // A variation of the main line: its own indented paragraph between two parts of the table.
    void DrawVariationBlock(Context& t_ctx, const GameState::NodeId t_first)
    {
        ImGui::Indent(VARIATION_INDENT);

        Flow flow;
        flow.right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        EmitLine(t_ctx, flow, t_first, false);

        ImGui::Unindent(VARIATION_INDENT);
    }

    void SetupColumns()
    {
        ImGui::TableSetupColumn("##number", ImGuiTableColumnFlags_WidthFixed, NUMBER_COLUMN_WIDTH);
        ImGui::TableSetupColumn("White", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Black", ImGuiTableColumnFlags_WidthStretch);
    }

    // The main line as a table with a row for each pair of moves. Where a move has alternatives, the table is
    // interrupted, the variations are written below the move, and the table goes on with a new part.
    void DrawMainLine(Context& t_ctx)
    {
        const GameState::Node& start{ t_ctx.game.At(0) };
        if (start.children.empty())
        {
            ImGui::TextDisabled("No moves yet. Move a piece on the board.");
            return;
        }

        std::optional<GameState::NodeId> id{ start.children.front() };
        bool tableOpen{ false };
        bool rowOpen{ false };
        int part{ 0 };

        while (id)
        {
            const GameState::Node& node{ t_ctx.game.At(*id) };
            const GameState::Node& parent{ t_ctx.game.At(*node.parent) };
            const bool whiteMoves{ parent.info.position.SideToMove() == sg::chess::core::Color::WHITE };

            if (!tableOpen)
            {
                const std::string name{ "##part" + std::to_string(part) };
                tableOpen = ImGui::BeginTable(name.c_str(), 3, TABLE_FLAGS);
                if (tableOpen)
                {
                    SetupColumns();
                }

                rowOpen = false;
            }

            // A new row starts with each White move, and with a Black move that does not follow a White move
            // in the table: the first move, or the one after a variation.
            if (tableOpen && (!rowOpen || whiteMoves))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled(whiteMoves ? "%d." : "%d...", parent.info.position.FullmoveNumber());
                rowOpen = true;
            }

            if (tableOpen)
            {
                ImGui::TableSetColumnIndex(whiteMoves ? 1 : 2);
                MoveItem(t_ctx, *id, ImVec2(0.0f, 0.0f), nullptr);
            }

            if (!whiteMoves)
            {
                rowOpen = false;
            }

            if (parent.children.size() > 1 && parent.children.front() == *id)
            {
                if (tableOpen)
                {
                    ImGui::EndTable();
                    tableOpen = false;
                }

                ++part;

                for (std::size_t i{ 1 }; i < parent.children.size(); ++i)
                {
                    DrawVariationBlock(t_ctx, parent.children[i]);
                }
            }

            id = node.children.empty() ? std::nullopt : std::optional{ node.children.front() };
        }

        if (tableOpen)
        {
            ImGui::EndTable();
        }
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
    // The header stays where it is while the list scrolls. It is a table of its own, as wide as the list below,
    // which always has a scroll bar.
    const float width{ ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize };
    if (ImGui::BeginTable("##header", 3, TABLE_FLAGS, ImVec2(width, 0.0f)))
    {
        SetupColumns();
        ImGui::TableHeadersRow();
        ImGui::EndTable();
    }

    Context ctx{ t_game, m_scrollToCurrent, {} };

    const ImVec2 size{ 0.0f, ImGui::GetContentRegionAvail().y - NavigationHeight() };
    if (ImGui::BeginChild("##notation", size, ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar))
    {
        DrawMainLine(ctx);
    }
    ImGui::EndChild();

    m_scrollToCurrent = false;

    // The game is changed only now, after it has been drawn.
    bool changed{ false };

    if (ctx.requests.select)
    {
        t_game.GoTo(*ctx.requests.select);
        changed = true;
    }

    if (ctx.requests.promote)
    {
        t_game.Promote(*ctx.requests.promote);
    }

    if (ctx.requests.remove)
    {
        t_game.DeleteFrom(*ctx.requests.remove);
        changed = true;
    }

    return changed;
}
