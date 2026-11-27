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
#include <array>
#include <exception>
#include <string_view>
#include <imgui.h>
#include <utility>
#include "core/DrawRules.h"
#include "core/I18n.h"
#include "core/Material.h"
#include "core/PgnTags.h"
#include "core/Util.h"
#include "ui/Layout.h"
#include "ui/screens/AnalysisScreen.h"

namespace
{
    constexpr std::array<std::string_view, 4> OPENING_TAGS{ "ECO", "Opening", "Variation", "SubVariation" };

    // True if the tags say what set_opening_tags() would write for this opening.
    bool OpeningTagsMatch(const sg::chess::core::PgnHeaders& t_headers, const sg::chess::core::Opening& t_opening)
    {
        sg::chess::core::PgnHeaders expected;
        sg::chess::core::set_opening_tags(expected, t_opening);

        return std::ranges::all_of(OPENING_TAGS, [&](const std::string_view t_name)
                                   { return expected.Get(t_name) == t_headers.Get(t_name); });
    }

    // The board is a little smaller than it could be: a strip with a player fits above and below it.
    constexpr float SQUARE_SIZE{ 72.0f };
    constexpr float MARGIN{ 30.0f };
    constexpr float TOP_MARGIN{ 14.0f };
    constexpr float STRIP_HEIGHT{ 46.0f };
    constexpr float STRIP_GAP{ 4.0f };
    constexpr float COORDINATE_ROW{ 22.0f };  // the files are written between the board and the lower strip
    constexpr float BOARD_TOP{ TOP_MARGIN + STRIP_HEIGHT + STRIP_GAP };

    constexpr float BOARD_PIXELS{ static_cast<float>(sg::chess::core::BOARD_SIZE) * SQUARE_SIZE };

    // The evaluation bar starts to the right of the navigation bar and is as tall as the board.
    constexpr float EVAL_BAR_WIDTH{ 36.0f };
    // The gap also holds the numbers of the ranks. The files are written below the board, where there is room.
    constexpr float EVAL_BAR_GAP{ 26.0f };
    constexpr sf::Vector2f EVAL_BAR_ORIGIN{ sg::chess::ui::NAVIGATION_WIDTH + MARGIN, BOARD_TOP };
    constexpr sf::Vector2f EVAL_BAR_SIZE{ EVAL_BAR_WIDTH, BOARD_PIXELS };

    // The board starts to the right of the evaluation bar.
    constexpr sf::Vector2f BOARD_ORIGIN{ EVAL_BAR_ORIGIN.x + EVAL_BAR_WIDTH + EVAL_BAR_GAP, BOARD_TOP };

    // The strip with the lower player is below the files.
    constexpr float LOWER_STRIP_TOP{ BOARD_TOP + BOARD_PIXELS + COORDINATE_ROW };

    // The side panel starts to the right of the board and is as tall as the board with the two strips.
    // The gap to the board is smaller than the margin, so the panel stays wide enough for its buttons.
    constexpr float PANEL_GAP{ 18.0f };
    constexpr float PANEL_LEFT{ BOARD_ORIGIN.x + BOARD_PIXELS + PANEL_GAP };
    constexpr float PANEL_TOP{ TOP_MARGIN };
    constexpr float PANEL_HEIGHT{ LOWER_STRIP_TOP + STRIP_HEIGHT - PANEL_TOP };

    constexpr ImGuiWindowFlags PANEL_FLAGS{ ImGuiWindowFlags_NoTitleBar |
                                            ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoMove |
                                            ImGuiWindowFlags_NoCollapse |
                                            ImGuiWindowFlags_NoSavedSettings };

    constexpr float HEADER_BUTTON_WIDTH{ 60.0f };
    constexpr const char* NEW_GAME_POPUP{ "###new_game" };
    constexpr const char* OPEN_GAME_POPUP{ "###open_game" };
    constexpr ImVec4 ERROR_COLOR{ 1.0f, 0.4f, 0.4f, 1.0f };
}

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::AnalysisScreen::AnalysisScreen(const PieceSet& t_pieces, core::RulesProvider& t_rules,
                                              engine::Analyzer& t_analyzer, const core::OpeningBook& t_openings,
                                              const core::Settings::Analysis& t_settings)
    : m_rules(t_rules)
    , m_game(t_rules)
    , m_boardView(BOARD_ORIGIN, SQUARE_SIZE, t_pieces)
    , m_input(m_boardView)
    , m_openings(t_openings)
    , m_evalBar(EVAL_BAR_ORIGIN, EVAL_BAR_SIZE)
    , m_enginePanel(t_pieces, t_rules, t_analyzer, t_settings.engineOnAtStart)
    , m_playerStrip(t_pieces)
    , m_pgnMenu(t_rules)
    , m_headers(core::PgnHeaders::MakeDefault(core::format_local_time("%Y.%m.%d")))
{
    m_enginePanel.PositionChanged(m_game.Current().info);
    UpdateOpening();
}

//-------------------------------------------------
// Games from the list
//-------------------------------------------------

void sg::chess::ui::AnalysisScreen::SetBackFunction(BackFunction t_back)
{
    m_back = std::move(t_back);
}

void sg::chess::ui::AnalysisScreen::OpenGame(core::GameSummary t_game)
{
    m_pending = std::move(t_game);
    m_asking = false;
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::AnalysisScreen::Title() const noexcept
{
    return core::tr("Analysis");
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

    // The way back is taken here and not in DrawGui(): switching pages while this page is being drawn is not safe.
    if (m_wantBack)
    {
        m_wantBack = false;

        if (m_back)
        {
            m_back();
        }
    }
}

void sg::chess::ui::AnalysisScreen::Draw(sf::RenderTarget& t_target)
{
    m_evalBar.Draw(t_target, m_boardView.IsFlipped());
    m_boardView.Draw(t_target, m_game.Current().info.position, BuildOverlay());
}

void sg::chess::ui::AnalysisScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    DrawPlayers();

    ImGui::SetNextWindowPos({ PANEL_LEFT, PANEL_TOP });
    ImGui::SetNextWindowSize({ displaySize.x - PANEL_LEFT - MARGIN, PANEL_HEIGHT });

    if (ImGui::Begin("##analysis", nullptr, PANEL_FLAGS))
    {
        // A game from the list may wait here. The board is not touched while its parts are drawn: this is first.
        DrawOpenQuestion();
        DrawBackBar();

        // Three zones from the top: the game (name, status), the engine, and the moves with the navigation.
        DrawHeader();
        ImGui::Separator();

        if (const auto clicked{ m_enginePanel.Draw(m_game.Current().info) })
        {
            // Playing a move changes the position, so it is done after the panel has been drawn.
            if (m_game.Play(*clicked))
            {
                OnPositionChanged();
            }
        }

        ImGui::Separator();

        // Above the list: the check box for the comments at the left, the PGN button at the right end.
        const bool toggleDrawn{ m_notationPanel.DrawCommentToggle(m_game) };
        const float pgnX{ ImGui::GetContentRegionMax().x - PgnMenu::ButtonWidth() };
        if (toggleDrawn)
        {
            ImGui::SameLine(pgnX);
        }
        else
        {
            ImGui::SetCursorPosX(pgnX);
        }

        // The tags are up to date before the game is copied or saved.
        UpdateOpeningTags();

        // The game as it was opened, with the opening tags. Everything that differs later is a change of the user.
        if (m_cleanPending)
        {
            m_cleanText = core::pgn_write(m_game, m_headers);
            m_cleanPending = false;
        }

        bool changed{ m_pgnMenu.Draw(m_game, m_headers) };
        if (changed)
        {
            // Another game has been opened: everything that belonged to the old one is dropped.
            m_notationPanel.Reset();
            m_cameFromList = false;
            m_listed.reset();
        }

        changed = m_notationPanel.DrawMoveList(m_game, m_headers) || changed;

        // The pencil in the header of the list opens the window for the details of the game.
        if (m_notationPanel.TakeDetailsRequest())
        {
            m_details.Open(m_headers);
        }

        static_cast<void>(m_details.Draw(m_headers));

        ImGui::Separator();
        changed = m_notationPanel.DrawNavigation(m_game) || changed;

        if (changed)
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

std::string sg::chess::ui::AnalysisScreen::StatusText(const std::optional<core::DrawReason>& t_draw) const
{
    const auto& info{ m_game.Current().info };
    const bool whiteToMove{ info.position.SideToMove() == core::Color::WHITE };

    if (info.IsCheckmate())
    {
        return core::tr_format("Checkmate - {} wins", whiteToMove ? core::tr("Black") : core::tr("White"));
    }

    if (info.IsStalemate())
    {
        return core::tr("Stalemate - draw");
    }

    // A draw that a player has to claim does not end the game: it is a hint, written below.
    if (t_draw && core::is_automatic_draw(*t_draw))
    {
        return core::tr_format("Draw - {}", core::describe_draw(*t_draw));
    }

    std::string text{ whiteToMove ? core::tr("White to move") : core::tr("Black to move") };

    if (info.inCheck)
    {
        text += core::tr(" (check)");
    }

    return text;
}

void sg::chess::ui::AnalysisScreen::UpdateOpening()
{
    m_opening.reset();

    // Walk back along the moves that led here until a position has a name. A game that has left the book
    // keeps the last name, and a variation finds the name of the line it belongs to.
    std::optional<core::GameState::NodeId> id{ m_game.CurrentId() };

    while (id)
    {
        const core::GameState::Node& node{ m_game.At(*id) };

        if (const auto opening{ m_openings.Find(node.info.position) })
        {
            m_opening = opening;
            return;
        }

        id = node.parent;
    }
}

void sg::chess::ui::AnalysisScreen::UpdateOpeningTags()
{
    // Along the main line from the start: the last position with a name.
    std::optional<core::Opening> found;
    core::GameState::NodeId id{ m_game.At(0).children.empty() ? 0 : m_game.At(0).children.front() };

    while (id != 0)
    {
        const core::GameState::Node& node{ m_game.At(id) };

        if (const auto opening{ m_openings.Find(node.info.position) })
        {
            found = opening;
        }

        id = node.children.empty() ? 0 : node.children.front();
    }

    if (found)
    {
        if (!OpeningTagsMatch(m_headers, *found))
        {
            core::set_opening_tags(m_headers, *found);
        }

        m_taggedOpening = found;
    }
    else
    {
        // Only tags that we wrote ourselves go away. Tags from a file stay.
        if (m_taggedOpening && OpeningTagsMatch(m_headers, *m_taggedOpening))
        {
            core::clear_opening_tags(m_headers);
        }

        m_taggedOpening.reset();
    }
}

void sg::chess::ui::AnalysisScreen::OnPositionChanged()
{
    m_input.Reset();
    m_notationPanel.ScrollToCurrent();
    m_enginePanel.PositionChanged(m_game.Current().info);
    UpdateOpening();
}

bool sg::chess::ui::AnalysisScreen::IsModified() const
{
    return m_game.HasMoves() && core::pgn_write(m_game, m_headers) != m_cleanText;
}

void sg::chess::ui::AnalysisScreen::LoadListedGame()
{
    if (!m_pending)
    {
        return;
    }

    m_cameFromList = true;

    try
    {
        // The game is built on its own. The one on the board is only replaced when this has worked.
        core::PgnGame loaded{ core::pgn_read(m_pending->pgn, m_rules) };

        m_game.ReplaceWith(std::move(loaded.game));
        m_headers = std::move(loaded.headers);
        m_game.GoToStart();
        m_notationPanel.Reset();

        // The user is at the bottom of the board. If the user is not in the game, the board stays as it is.
        if (m_pending->userColor)
        {
            m_boardView.SetFlipped(*m_pending->userColor == core::Color::BLACK);
        }

        m_listed = std::move(m_pending);
        m_cleanPending = true;
        m_openError.clear();
        OnPositionChanged();
    }
    catch (const std::exception& t_error)
    {
        m_openError = t_error.what();
    }

    m_pending.reset();
    m_asking = false;
}

void sg::chess::ui::AnalysisScreen::DrawOpenQuestion()
{
    if (m_pending && !m_asking)
    {
        // Own work is not thrown away without a question.
        if (IsModified())
        {
            ImGui::OpenPopup(OPEN_GAME_POPUP);
            m_asking = true;
        }
        else
        {
            LoadListedGame();
        }
    }

    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };
    ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(core::tr_id("Open game", OPEN_GAME_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(core::tr("The game on the board has changes (moves, variations or comments)."));
        ImGui::TextUnformatted(core::tr("Replace it with the game from the list?"));
        ImGui::Spacing();

        if (ImGui::Button(core::tr("Replace")))
        {
            LoadListedGame();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        // The Escape key cancels, like the button. The game on the board stays.
        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_pending.reset();
            m_asking = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void sg::chess::ui::AnalysisScreen::DrawBackBar()
{
    if (!m_cameFromList || !m_back)
    {
        return;
    }

    ImGui::AlignTextToFramePadding();

    if (ImGui::Button(core::tr("< My Games")))
    {
        m_wantBack = true;
    }

    // The accuracy is a number that the website has worked out. It is shown as what it is, with its source.
    if (m_listed && m_listed->whiteAccuracy && m_listed->blackAccuracy)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("%s",
                            core::tr_format("Accuracy (chess.com): {:.1f}% / {:.1f}%", *m_listed->whiteAccuracy,
                                            *m_listed->blackAccuracy)
                                .c_str());
    }

    if (!m_openError.empty())
    {
        ImGui::TextColored(ERROR_COLOR, "%s", core::tr("This game could not be opened:"));
        ImGui::TextWrapped("%s", m_openError.c_str());
    }

    ImGui::Separator();
}

void sg::chess::ui::AnalysisScreen::DrawPlayers()
{
    const core::Material material{ core::captured_material(m_game) };

    // The side that is at the bottom of the board has the lower strip.
    const auto strip = [this, &material](const core::Color t_color, const float t_y)
    {
        const bool white{ t_color == core::Color::WHITE };

        m_playerStrip.Draw(ImVec2(BOARD_ORIGIN.x, t_y), STRIP_HEIGHT, t_color, m_headers.PlayerName(t_color),
                           m_headers.PlayerRating(t_color), white ? material.byWhite : material.byBlack,
                           white ? material.Balance() : -material.Balance());
    };

    const bool flipped{ m_boardView.IsFlipped() };
    strip(flipped ? core::Color::WHITE : core::Color::BLACK, TOP_MARGIN);
    strip(flipped ? core::Color::BLACK : core::Color::WHITE, LOWER_STRIP_TOP);
}

void sg::chess::ui::AnalysisScreen::DrawHeader()
{
    // The name of the opening, or a neutral title if the game has none (yet).
    if (m_opening)
    {
        ImGui::TextDisabled("%s", m_opening->eco.c_str());
        ImGui::SameLine();
        ImGui::TextWrapped("%s", m_opening->name.c_str());
    }
    else
    {
        ImGui::TextUnformatted(core::tr("Analysis board"));
    }

    // The status on the left, the buttons at the right end of the same line.
    // The repetitions are counted along the moves that lead to the position, so the whole game is needed.
    const int repetitions{ core::repetition_count(m_game, m_game.CurrentId()) };
    const std::optional<core::DrawReason> draw{ core::find_draw(m_game.Current().info, repetitions) };

    const std::string status{ StatusText(draw) };
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(status.c_str());

    const float spacing{ ImGui::GetStyle().ItemSpacing.x };
    ImGui::SameLine(ImGui::GetContentRegionMax().x - 2.0f * HEADER_BUTTON_WIDTH - spacing);

    if (ImGui::Button(core::tr("Flip"), ImVec2(HEADER_BUTTON_WIDTH, 0.0f)))
    {
        m_boardView.Flip();
    }

    ImGui::SameLine();

    // A game with moves is not thrown away by one click.
    if (ImGui::Button(core::tr("New"), ImVec2(HEADER_BUTTON_WIDTH, 0.0f)))
    {
        if (!m_game.HasMoves())
        {
            m_game.Reset();
            m_headers = core::PgnHeaders::MakeDefault(core::format_local_time("%Y.%m.%d"));
            OnPositionChanged();
        }
        else
        {
            ImGui::OpenPopup(NEW_GAME_POPUP);
        }
    }

    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };
    ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(core::tr_id("New game", NEW_GAME_POPUP).c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextUnformatted(core::tr("Throw away the moves and start a new game?"));
        ImGui::Spacing();

        if (ImGui::Button(core::tr("Start new game")))
        {
            m_game.Reset();
            m_headers = core::PgnHeaders::MakeDefault(core::format_local_time("%Y.%m.%d"));
            m_notationPanel.Reset();
            m_cameFromList = false;
            m_listed.reset();
            OnPositionChanged();
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        // The Escape key cancels, like the button.
        if (ImGui::Button(core::tr("Cancel")) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    // Threefold repetition and the 50-move rule: a player may claim a draw.
    if (draw && !core::is_automatic_draw(*draw))
    {
        ImGui::TextDisabled("%s", core::tr_format("A draw can be claimed: {}", core::describe_draw(*draw)).c_str());
    }
}
