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
#include <optional>
#include <sstream>
#include <string>
#include "ui/Icons.h"
#include "ui/NotationPanel.h"

namespace
{
    using GameState = sg::chess::core::GameState;
    using CommentKind = sg::chess::ui::NotationPanel::CommentKind;
    using CommentRef = sg::chess::ui::NotationPanel::CommentRef;

    constexpr float NUMBER_COLUMN_WIDTH{ 56.0f };
    constexpr float BUTTON_WIDTH{ 64.0f };

    // A variation is indented, and the closing parenthesis of a nested one sits close to the last move.
    constexpr float VARIATION_INDENT{ 14.0f };
    constexpr float PARENTHESIS_SPACING{ 2.0f };

    // The bar at the left of a variation shows at a glance which lines belong together.
    constexpr float BAR_OFFSET{ 2.0f };
    constexpr float BAR_WIDTH{ 3.0f };
    constexpr ImU32 BAR_COLOR{ IM_COL32(90, 150, 220, 255) };

    // Comments on a move are yellow, comments on a line or on the game are blue, like the bar.
    constexpr unsigned int MOVE_COMMENT_COLOR{ IM_COL32(235, 200, 80, 255) };
    constexpr unsigned int LINE_COMMENT_COLOR{ IM_COL32(110, 170, 235, 255) };
    constexpr float COMMENT_INDENT{ 8.0f };
    constexpr float TOOLTIP_WIDTH_IN_CHARACTERS{ 24.0f };

    constexpr const char* EDIT_POPUP{ "Comment" };
    constexpr float EDIT_WIDTH{ 420.0f };
    constexpr float EDIT_HEIGHT{ 130.0f };

    constexpr ImGuiTableFlags TABLE_FLAGS{ ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                                           ImGuiTableFlags_NoSavedSettings };

    // The line below the table: a separator and a row of buttons.
    float NavigationHeight()
    {
        return ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2.0f + 1.0f;
    }

    //-------------------------------------------------
    // Comments
    //-------------------------------------------------

    // A move as it is written in front of a variation: "12. Nf3" or "12... Nf6".
    std::string MoveLabel(const GameState& t_game, const GameState::NodeId t_id)
    {
        const GameState::Node& node{ t_game.At(t_id) };
        const GameState::Node& parent{ t_game.At(*node.parent) };
        const bool whiteMoves{ parent.info.position.SideToMove() == sg::chess::core::Color::WHITE };

        return std::to_string(parent.info.position.FullmoveNumber()) + (whiteMoves ? ". " : "... ") + node.san;
    }

    const std::string& CommentText(const GameState& t_game, const CommentRef t_ref)
    {
        switch (t_ref.kind)
        {
        case CommentKind::MOVE:
            return t_game.At(t_ref.id).note.comment;
        case CommentKind::LINE:
            return t_game.At(t_ref.id).lineComment;
        case CommentKind::GAME:
            break;
        }

        return t_game.At(0).note.comment;
    }

    std::uint64_t KeyOf(const CommentRef t_ref)
    {
        return static_cast<std::uint64_t>(t_ref.id) * 4 + static_cast<std::uint64_t>(t_ref.kind);
    }

    bool HasComments(const GameState& t_game)
    {
        for (GameState::NodeId id{ 0 }; id < t_game.NodeCount(); ++id)
        {
            if (!t_game.At(id).note.comment.empty() || !t_game.At(id).lineComment.empty())
            {
                return true;
            }
        }

        return false;
    }

    //-------------------------------------------------
    // Drawing
    //-------------------------------------------------

    // What the user asked for while the list was drawn. It is carried out afterwards, because it changes the game
    // that is being drawn.
    struct Requests
    {
        std::optional<GameState::NodeId> select;   // go to this move
        std::optional<GameState::NodeId> promote;  // make this line the main line
        std::optional<GameState::NodeId> remove;   // delete this move and everything after it
        std::optional<CommentRef> edit;            // write this comment
        std::optional<CommentRef> toggle;          // show or hide this comment in full
    };

    struct Context
    {
        const GameState& game;
        bool scrollToCurrent;
        bool showAllComments;
        const std::unordered_set<std::uint64_t>& openComments;
        Requests requests;

        // A comment is shown in full if all are, or if it was opened.
        [[nodiscard]] bool IsShown(const CommentRef t_ref) const
        {
            return showAllComments || openComments.contains(KeyOf(t_ref));
        }
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
            if (ImGui::MenuItem(node.note.comment.empty() ? "Add comment..." : "Edit comment..."))
            {
                t_ctx.requests.edit = CommentRef{ CommentKind::MOVE, t_id };
            }

            // The first move of a variation can have a comment on the whole variation.
            const bool startsVariation{ t_ctx.game.At(*node.parent).children.front() != t_id };
            const char* lineLabel{ node.lineComment.empty() ? "Add comment on variation..."
                                                            : "Edit comment on variation..." };
            if (startsVariation && ImGui::MenuItem(lineLabel))
            {
                t_ctx.requests.edit = CommentRef{ CommentKind::LINE, t_id };
            }

            ImGui::Separator();

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

    // The speech bubble of a comment. Over it, the comment is shown. A click shows it in full, or opens the window to
    // write it if there is none yet and t_allowAdd is set. Without a comment and without t_allowAdd nothing is drawn.
    void CommentIcon(Context& t_ctx, const CommentRef t_ref, const bool t_allowAdd, Flow* t_flow)
    {
        const std::string& text{ CommentText(t_ctx.game, t_ref) };
        if (text.empty() && !t_allowAdd)
        {
            return;
        }

        const float size{ ImGui::GetTextLineHeight() };
        if (t_flow != nullptr)
        {
            Place(*t_flow, size, ImGui::GetStyle().ItemSpacing.x);
        }

        ImGui::PushID(static_cast<int>(KeyOf(t_ref)));

        const unsigned int color{ t_ref.kind == CommentKind::MOVE ? MOVE_COMMENT_COLOR : LINE_COMMENT_COLOR };
        const bool clicked{ sg::chess::ui::comment_icon(!text.empty(), t_ctx.IsShown(t_ref), size, color) };

        if (t_flow != nullptr)
        {
            t_flow->lastMaxX = ImGui::GetItemRectMax().x;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip) && ImGui::BeginTooltip())
        {
            if (text.empty())
            {
                ImGui::TextUnformatted(t_ref.kind == CommentKind::MOVE ? "Add a comment"
                                                                       : "Add a comment on this line");
            }
            else
            {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * TOOLTIP_WIDTH_IN_CHARACTERS);
                ImGui::TextUnformatted(text.c_str());
                ImGui::PopTextWrapPos();
            }

            ImGui::EndTooltip();
        }

        ImGui::PopID();

        if (clicked)
        {
            if (text.empty())
            {
                t_ctx.requests.edit = t_ref;
            }
            else
            {
                t_ctx.requests.toggle = t_ref;
            }
        }
    }

    // Text that cannot be clicked (a move number, a parenthesis, a word of a comment) as an item of the flow.
    void TextToken(Flow& t_flow, const std::string& t_text, const float t_spacing, const ImU32 t_color)
    {
        Place(t_flow, ImGui::CalcTextSize(t_text.c_str()).x, t_spacing);

        ImGui::PushStyleColor(ImGuiCol_Text, t_color);
        ImGui::TextUnformatted(t_text.c_str());
        ImGui::PopStyleColor();

        t_flow.lastMaxX = ImGui::GetItemRectMax().x;
    }

    // A comment that is shown in full inside a variation: its words flow like the moves.
    void CommentWords(Flow& t_flow, const std::string& t_text, const ImU32 t_color)
    {
        std::istringstream words{ t_text };
        std::string word;

        while (words >> word)
        {
            TextToken(t_flow, word, ImGui::GetStyle().ItemSpacing.x, t_color);
        }
    }

    // A comment that is shown in full between two parts of the table: a paragraph.
    void CommentBlock(const std::string& t_text, const ImU32 t_color)
    {
        ImGui::Indent(COMMENT_INDENT);
        ImGui::PushStyleColor(ImGuiCol_Text, t_color);
        ImGui::TextWrapped("%s", t_text.c_str());
        ImGui::PopStyleColor();
        ImGui::Unindent(COMMENT_INDENT);
    }

    // Writes a line of moves into the flow, with move numbers and comments. The alternatives to a move follow it
    // in parentheses. A nested line starts with an opening parenthesis and the bubble of its comment.
    void EmitLine(Context& t_ctx, Flow& t_flow, const GameState::NodeId t_first, const bool t_nested)
    {
        const float spacing{ ImGui::GetStyle().ItemSpacing.x };
        const ImU32 numberColor{ ImGui::GetColorU32(ImGuiCol_TextDisabled) };
        const ImU32 moveColor{ MOVE_COMMENT_COLOR };
        const ImU32 lineColor{ LINE_COMMENT_COLOR };

        // The space in front of the next token: after an opening parenthesis it is small.
        float nextSpacing{ spacing };
        bool needNumber{ true };

        if (t_nested)
        {
            TextToken(t_flow, "(", spacing, numberColor);
            nextSpacing = PARENTHESIS_SPACING;

            const CommentRef line{ CommentKind::LINE, t_first };
            CommentIcon(t_ctx, line, false, &t_flow);

            if (!CommentText(t_ctx.game, line).empty() && t_ctx.IsShown(line))
            {
                CommentWords(t_flow, CommentText(t_ctx.game, line), lineColor);
            }
        }

        std::optional<GameState::NodeId> id{ t_first };

        while (id)
        {
            const GameState::Node& node{ t_ctx.game.At(*id) };
            const GameState::Node& parent{ t_ctx.game.At(*node.parent) };
            const bool whiteMoves{ parent.info.position.SideToMove() == sg::chess::core::Color::WHITE };
            const std::string number{ std::to_string(parent.info.position.FullmoveNumber()) };

            // White's moves get a number. A Black move only does at the start of a line and after a variation.
            if (whiteMoves || needNumber)
            {
                TextToken(t_flow, number + (whiteMoves ? "." : "..."), nextSpacing, numberColor);
                nextSpacing = spacing;
            }

            needNumber = false;

            const float width{ ImGui::CalcTextSize(node.san.c_str()).x };
            Place(t_flow, width, nextSpacing);
            nextSpacing = spacing;
            MoveItem(t_ctx, *id, ImVec2(width, 0.0f), &t_flow);

            const CommentRef comment{ CommentKind::MOVE, *id };
            CommentIcon(t_ctx, comment, false, &t_flow);

            if (!node.note.comment.empty() && t_ctx.IsShown(comment))
            {
                CommentWords(t_flow, node.note.comment, moveColor);
                needNumber = true;
            }

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
            TextToken(t_flow, ")", PARENTHESIS_SPACING, numberColor);
        }
    }

    // A variation of the main line: a block of its own between two parts of the table. The bar at the left and
    // the header "Alternative to 1. e4" show to which move it belongs. The bubble in the header is the comment on
    // the variation.
    void DrawVariationBlock(Context& t_ctx, const GameState::NodeId t_first)
    {
        const GameState::Node& node{ t_ctx.game.At(t_first) };
        const GameState::NodeId mainMove{ t_ctx.game.At(*node.parent).children.front() };

        const ImVec2 top{ ImGui::GetCursorScreenPos() };
        ImGui::Indent(VARIATION_INDENT);

        ImGui::TextDisabled("Alternative to %s", MoveLabel(t_ctx.game, mainMove).c_str());
        ImGui::SameLine();

        const CommentRef line{ CommentKind::LINE, t_first };
        CommentIcon(t_ctx, line, true, nullptr);

        if (!node.lineComment.empty() && t_ctx.IsShown(line))
        {
            CommentBlock(node.lineComment, LINE_COMMENT_COLOR);
        }

        Flow flow;
        flow.right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        EmitLine(t_ctx, flow, t_first, false);

        ImGui::Unindent(VARIATION_INDENT);

        const float bottom{ ImGui::GetCursorScreenPos().y - ImGui::GetStyle().ItemSpacing.y };
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(top.x + BAR_OFFSET, top.y),
                                                  ImVec2(top.x + BAR_OFFSET + BAR_WIDTH, bottom), BAR_COLOR);
    }

    void SetupColumns()
    {
        ImGui::TableSetupColumn("##number", ImGuiTableColumnFlags_WidthFixed, NUMBER_COLUMN_WIDTH);
        ImGui::TableSetupColumn("White", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Black", ImGuiTableColumnFlags_WidthStretch);
    }

    // The header of the main line, with the bubble of the comment on the game.
    void DrawMainLineHeader(Context& t_ctx)
    {
        ImGui::TextDisabled("Main line");
        ImGui::SameLine();

        const CommentRef game{ CommentKind::GAME, 0 };
        CommentIcon(t_ctx, game, true, nullptr);

        if (!CommentText(t_ctx.game, game).empty() && t_ctx.IsShown(game))
        {
            CommentBlock(CommentText(t_ctx.game, game), LINE_COMMENT_COLOR);
        }
    }

    // The main line as a table with a row for each pair of moves. Where a move has alternatives or an open comment,
    // the table is interrupted, they are written below the move, and the table goes on with a new part.
    void DrawMainLine(Context& t_ctx)
    {
        DrawMainLineHeader(t_ctx);

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
            // in the table: the first move, or the one after a variation or a comment.
            if (tableOpen && (!rowOpen || whiteMoves))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled(whiteMoves ? "%d." : "%d...", parent.info.position.FullmoveNumber());
                rowOpen = true;
            }

            const CommentRef comment{ CommentKind::MOVE, *id };
            const bool hasComment{ !node.note.comment.empty() };

            if (tableOpen)
            {
                ImGui::TableSetColumnIndex(whiteMoves ? 1 : 2);

                // With a bubble, the move is a little narrower to leave room for it.
                const float iconSpace{ ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.x };
                const float width{ hasComment ? std::max(ImGui::GetContentRegionAvail().x - iconSpace, 0.0f) : 0.0f };
                MoveItem(t_ctx, *id, ImVec2(width, 0.0f), nullptr);

                if (hasComment)
                {
                    ImGui::SameLine();
                    CommentIcon(t_ctx, comment, false, nullptr);
                }
            }

            if (!whiteMoves)
            {
                rowOpen = false;
            }

            const bool commentShown{ hasComment && t_ctx.IsShown(comment) };
            const bool hasVariations{ parent.children.size() > 1 && parent.children.front() == *id };

            if (commentShown || hasVariations)
            {
                if (tableOpen)
                {
                    ImGui::EndTable();
                    tableOpen = false;
                }

                ++part;

                if (commentShown)
                {
                    CommentBlock(node.note.comment, MOVE_COMMENT_COLOR);
                }

                if (hasVariations)
                {
                    for (std::size_t i{ 1 }; i < parent.children.size(); ++i)
                    {
                        DrawVariationBlock(t_ctx, parent.children[i]);
                    }
                }

                rowOpen = false;
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
    // The check box is only there if there is something to show.
    if (HasComments(t_game))
    {
        ImGui::Checkbox("Show all comments", &m_showAllComments);
    }

    // The header stays where it is while the list scrolls. It is a table of its own, as wide as the list below,
    // which always has a scroll bar.
    const float width{ ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize };
    if (ImGui::BeginTable("##header", 3, TABLE_FLAGS, ImVec2(width, 0.0f)))
    {
        SetupColumns();
        ImGui::TableHeadersRow();
        ImGui::EndTable();
    }

    Context ctx{ t_game, m_scrollToCurrent, m_showAllComments, m_openComments, {} };

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

    if (ctx.requests.toggle)
    {
        const std::uint64_t key{ KeyOf(*ctx.requests.toggle) };

        if (m_openComments.erase(key) == 0)
        {
            m_openComments.insert(key);
        }
    }

    if (ctx.requests.edit)
    {
        BeginEdit(t_game, *ctx.requests.edit);
    }

    if (ctx.requests.remove)
    {
        t_game.DeleteFrom(*ctx.requests.remove);

        // The ids of the other moves change, so what was opened by id is not valid any more.
        m_openComments.clear();
        changed = true;
    }

    DrawEditor(t_game);

    return changed;
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

void sg::chess::ui::NotationPanel::BeginEdit(const core::GameState& t_game, const CommentRef t_ref)
{
    m_editTarget = t_ref;

    const std::string& text{ CommentText(t_game, t_ref) };
    const std::size_t length{ std::min(text.size(), m_editBuffer.size() - 1) };

    std::memcpy(m_editBuffer.data(), text.data(), length);
    m_editBuffer[length] = '\0';

    ImGui::OpenPopup(EDIT_POPUP);
}

void sg::chess::ui::NotationPanel::DrawEditor(core::GameState& t_game)
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };
    ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal(EDIT_POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        return;
    }

    switch (m_editTarget.kind)
    {
    case CommentKind::MOVE:
        ImGui::Text("Comment on %s", MoveLabel(t_game, m_editTarget.id).c_str());
        break;
    case CommentKind::LINE:
        ImGui::Text("Comment on the variation that starts with %s", MoveLabel(t_game, m_editTarget.id).c_str());
        break;
    case CommentKind::GAME:
        ImGui::TextUnformatted("Comment on the main line and the whole game");
        break;
    }

    if (ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere();
    }

    ImGui::InputTextMultiline("##comment", m_editBuffer.data(), m_editBuffer.size(), ImVec2(EDIT_WIDTH, EDIT_HEIGHT));
    ImGui::Spacing();

    if (ImGui::Button("Save"))
    {
        // An empty comment removes it.
        std::string text{ m_editBuffer.data() };
        const std::size_t first{ text.find_first_not_of(" \t\r\n") };
        text = first == std::string::npos ? "" : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);

        switch (m_editTarget.kind)
        {
        case CommentKind::MOVE:
        case CommentKind::GAME:
        {
            const core::GameState::NodeId id{ m_editTarget.kind == CommentKind::GAME ? 0 : m_editTarget.id };

            core::GameState::Annotation note{ t_game.At(id).note };
            note.comment = std::move(text);
            t_game.SetAnnotation(id, std::move(note));
            break;
        }
        case CommentKind::LINE:
            t_game.SetLineComment(m_editTarget.id, std::move(text));
            break;
        }

        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    // The Escape key cancels, like the button.
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
