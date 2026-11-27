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
#include <array>
#include <cstring>
#include <format>
#include <string_view>
#include <utility>
#include "core/ChessComParser.h"
#include "core/I18n.h"
#include "ui/Layout.h"
#include "ui/screens/MyGamesScreen.h"

namespace
{
    namespace core = sg::chess::core;

    constexpr ImGuiWindowFlags WINDOW_FLAGS{ ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                                             | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                             | ImGuiWindowFlags_NoSavedSettings };

    // Layout of the page, in pixels.
    constexpr float WINDOW_PADDING{ 20.0f };
    constexpr float USERNAME_WIDTH{ 240.0f };
    constexpr float BUTTON_WIDTH{ 130.0f };
    constexpr float SEARCH_WIDTH{ 260.0f };
    constexpr float DISC_RADIUS{ 6.0f };

    // The months that "Load more" adds. Fetching is slow (one request after the other), so it is done in steps.
    constexpr int FIRST_MONTHS{ 3 };
    constexpr int MORE_MONTHS{ 6 };

    // The colors of the result column. They are used as a light tint of the cell and as the color of the text.
    constexpr ImVec4 WIN_COLOR{ 0.40f, 0.78f, 0.50f, 1.0f };
    constexpr ImVec4 LOSS_COLOR{ 0.92f, 0.45f, 0.45f, 1.0f };
    constexpr ImVec4 DRAW_COLOR{ 0.72f, 0.72f, 0.76f, 1.0f };
    constexpr ImVec4 ERROR_COLOR{ 1.0f, 0.4f, 0.4f, 1.0f };
    constexpr float TINT_ALPHA{ 0.16f };

    // The columns of the table.
    constexpr int COLUMN_COUNT{ 9 };
    constexpr float RESULT_WIDTH{ 96.0f };
    constexpr float COLOR_WIDTH{ 66.0f };
    constexpr float TIME_WIDTH{ 64.0f };
    constexpr float ACCURACY_WIDTH{ 100.0f };
    constexpr float MOVES_WIDTH{ 56.0f };
    constexpr float DATE_WIDTH{ 92.0f };
    constexpr float ACTION_WIDTH{ 118.0f };

    // The texts of the tabs. They are marked here and translated when they are drawn.
    struct TabText
    {
        std::string_view timeClass;
        const char* label;
    };

    constexpr std::array<TabText, 4> TABS{ { { "bullet", core::tr_noop("Bullet") },
                                             { "blitz", core::tr_noop("Blitz") },
                                             { "rapid", core::tr_noop("Rapid") },
                                             { "daily", core::tr_noop("Daily") } } };

    ImVec4 OutcomeColor(const core::Outcome t_outcome)
    {
        switch (t_outcome)
        {
        case core::Outcome::WIN:
            return WIN_COLOR;
        case core::Outcome::LOSS:
            return LOSS_COLOR;
        default:
            return DRAW_COLOR;
        }
    }

    const char* OutcomeText(const core::Outcome t_outcome)
    {
        switch (t_outcome)
        {
        case core::Outcome::WIN:
            return core::tr("Win");
        case core::Outcome::LOSS:
            return core::tr("Loss");
        case core::Outcome::DRAW:
            return core::tr("Draw");
        default:
            return "-";
        }
    }

    // What the user has to read when a fetch has gone wrong.
    std::string ErrorText(const sg::chess::net::FetchError t_error, const bool t_hasGames)
    {
        using sg::chess::net::FetchError;

        switch (t_error)
        {
        case FetchError::INVALID_USERNAME:
            return core::tr("This is not a possible user name on chess.com.");
        case FetchError::USER_NOT_FOUND:
            return core::tr("chess.com does not know a user with this name.");
        case FetchError::RATE_LIMITED:
            return core::tr("chess.com says that there are too many requests. Please try again in a few minutes.");
        case FetchError::NETWORK:
            return t_hasGames ? core::tr("There is no connection to chess.com. Games that were saved are shown.")
                              : core::tr("There is no connection to chess.com.");
        case FetchError::SERVER:
            return core::tr("chess.com has answered with an error.");
        case FetchError::BAD_RESPONSE:
            return core::tr("The answer of chess.com could not be read.");
        case FetchError::CANCELLED:
            return core::tr("Stopped.");
        default:
            return {};
        }
    }

    // The accuracy of one side as a text, or "-" if there is none.
    std::string AccuracyText(const std::optional<double>& t_value)
    {
        if (!t_value)
        {
            return "-";
        }

        return std::format("{:.1f}", *t_value);
    }
} // namespace

//-------------------------------------------------
// Ctors. / Dtor.
//-------------------------------------------------

sg::chess::ui::MyGamesScreen::MyGamesScreen(core::Settings& t_settings, net::GamesSyncer& t_syncer,
                                            OpenFunction t_open, ApplyFunction t_apply)
    : m_settings(t_settings)
    , m_syncer(t_syncer)
    , m_open(std::move(t_open))
    , m_apply(std::move(t_apply))
    , m_months(FIRST_MONTHS)
{
    std::strncpy(m_username.data(), m_settings.accounts.chesscom.username.c_str(), m_username.size() - 1);
}

//-------------------------------------------------
// Screen
//-------------------------------------------------

std::string_view sg::chess::ui::MyGamesScreen::Title() const noexcept
{
    return core::tr("My Games");
}

void sg::chess::ui::MyGamesScreen::OnEnter()
{
    // Once per session the games of the default user are fetched without a click: this page is made for them.
    if (m_firstEnter)
    {
        m_firstEnter = false;

        if (core::is_valid_chesscom_username(m_username.data()))
        {
            StartSync(false);
        }
    }
}

void sg::chess::ui::MyGamesScreen::Update(const sf::Time)
{
    if (auto result{ m_syncer.TakeResult() })
    {
        OnResult(std::move(*result));
    }

    // The game is handed over here and not while the table is drawn: the page is changed by this call.
    if (m_analyze)
    {
        const std::size_t index{ *m_analyze };
        m_analyze.reset();

        if (index < m_games.size() && m_open)
        {
            m_open(m_games[index]);
        }
    }
}

void sg::chess::ui::MyGamesScreen::DrawGui()
{
    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };

    // The page stands a little off the edges of the window, like the side panel of the analysis page.
    ImGui::SetNextWindowPos({ NAVIGATION_WIDTH + PAGE_MARGIN_SIDE, PAGE_MARGIN_TOP });
    ImGui::SetNextWindowSize({ displaySize.x - NAVIGATION_WIDTH - 2.0f * PAGE_MARGIN_SIDE,
                               displaySize.y - PAGE_MARGIN_TOP - PAGE_MARGIN_BOTTOM });

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(WINDOW_PADDING, WINDOW_PADDING));
    const bool open{ ImGui::Begin("##content", nullptr, WINDOW_FLAGS) };
    ImGui::PopStyleVar();

    if (open)
    {
        DrawTop();

        if (DrawFilter())
        {
            m_visibleOutdated = true;
        }

        DrawTable();
    }

    ImGui::End();
}

//-------------------------------------------------
// Helper
//-------------------------------------------------

void sg::chess::ui::MyGamesScreen::StartSync(const bool t_refreshAll)
{
    const std::string name{ m_username.data() };

    if (!core::is_valid_chesscom_username(name))
    {
        m_message = ErrorText(net::FetchError::INVALID_USERNAME, false);
        m_messageIsError = true;

        return;
    }

    // Another user starts with a short list again.
    if (name != m_loadedUser)
    {
        m_months = FIRST_MONTHS;
    }

    if (m_syncer.Start(name, m_months, t_refreshAll))
    {
        m_loadedUser = name;
        m_message.clear();
        m_messageIsError = false;
    }
}

void sg::chess::ui::MyGamesScreen::OnResult(net::FetchResult t_result)
{
    switch (t_result.error)
    {
    case net::FetchError::NONE:
        m_games = std::move(t_result.games);
        break;
    case net::FetchError::USER_NOT_FOUND:
    case net::FetchError::INVALID_USERNAME:
        // Nobody of this name: there are no games to show.
        m_games.clear();
        break;
    case net::FetchError::CANCELLED:
        // What the user stopped has only some of the months. The list from before stays.
        break;
    default:
        // The network or the server has failed. What the cache has is better than nothing.
        if (!t_result.games.empty())
        {
            m_games = std::move(t_result.games);
        }

        break;
    }

    if (t_result.error != net::FetchError::CANCELLED)
    {
        m_moreMonths = t_result.moreMonthsAvailable;
    }

    m_visibleOutdated = true;
    m_analyze.reset();

    if (t_result.error != net::FetchError::NONE)
    {
        m_message = ErrorText(t_result.error, !m_games.empty());
        m_messageIsError = t_result.error != net::FetchError::CANCELLED;

        return;
    }

    m_message = core::tr_format("{} games", m_games.size());

    if (t_result.skippedVariants > 0)
    {
        m_message += " "
                     + core::tr_format("({} games of other variants, e.g. Chess960, are not shown)",
                                       t_result.skippedVariants);
    }

    m_messageIsError = false;
}

void sg::chess::ui::MyGamesScreen::DrawTop()
{
    ImGui::SeparatorText(core::tr("My Games"));

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(core::tr("Chess.com username"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(USERNAME_WIDTH);

    const bool running{ m_syncer.IsRunning() };
    ImGui::BeginDisabled(running);

    // Enter in the field is the same as the button.
    const bool entered{ ImGui::InputText("##username", m_username.data(), m_username.size(),
                                         ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue) };
    ImGui::SameLine();

    if (ImGui::Button(core::tr("Sync Games"), ImVec2(BUTTON_WIDTH, 0.0f)) || entered)
    {
        StartSync(false);
    }

    ImGui::EndDisabled();

    if (running)
    {
        ImGui::SameLine();

        if (ImGui::Button(core::tr("Cancel"), ImVec2(BUTTON_WIDTH, 0.0f)))
        {
            m_syncer.Cancel();
        }

        const net::FetchProgress progress{ m_syncer.Progress() };
        ImGui::SameLine();

        if (progress.total > 0)
        {
            const std::string text{ core::tr_format("Month {} of {} ({})", std::min(progress.done + 1, progress.total),
                                                    progress.total, progress.month) };
            ImGui::TextDisabled("%s", text.c_str());
        }
        else
        {
            ImGui::TextDisabled("%s", core::tr("Asking chess.com ..."));
        }
    }
    else
    {
        // Another name for this session is only typed. To make it the default, the user says so.
        const std::string name{ m_username.data() };

        if (name != m_settings.accounts.chesscom.username && core::is_valid_chesscom_username(name))
        {
            ImGui::SameLine();

            if (ImGui::Button(core::tr("Use as default")))
            {
                m_settings.accounts.chesscom.username = name;

                if (const auto problem{ m_apply ? m_apply() : std::nullopt })
                {
                    m_message = core::tr_format("The settings could not be saved: {}", *problem);
                    m_messageIsError = true;
                }
            }
        }
    }

    if (!m_message.empty())
    {
        if (m_messageIsError)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ERROR_COLOR);
            ImGui::TextWrapped("%s", m_message.c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("%s", m_message.c_str());
            ImGui::PopStyleColor();
        }
    }

    ImGui::Spacing();
}

bool sg::chess::ui::MyGamesScreen::DrawFilter()
{
    bool changed{ false };
    std::string chosen;

    // One tab for "All" and one for each time class that there are games of.
    if (ImGui::BeginTabBar("##classes"))
    {
        if (ImGui::BeginTabItem(core::tr("All")))
        {
            ImGui::EndTabItem();
        }

        for (const std::string& timeClass : core::time_classes(m_games))
        {
            const auto tab{ std::ranges::find_if(
                TABS, [&](const TabText& t_tab) { return t_tab.timeClass == timeClass; }) };

            if (tab != TABS.end() && ImGui::BeginTabItem(core::tr(tab->label)))
            {
                chosen = timeClass;
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    if (chosen != m_filter.timeClass)
    {
        m_filter.timeClass = chosen;
        changed = true;
    }

    ImGui::SetNextItemWidth(SEARCH_WIDTH);
    ImGui::InputTextWithHint("##search", core::tr("Search opponent or opening"), m_search.data(), m_search.size());

    if (m_filter.text != m_search.data())
    {
        m_filter.text = m_search.data();
        changed = true;
    }

    return changed;
}

void sg::chess::ui::MyGamesScreen::DrawTable()
{
    // A click on a header sorts by that column, a second click turns the order around, a third one removes it. With
    // the Ctrl key, more columns can be combined.
    constexpr ImGuiTableFlags flags{ ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH
                                     | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit
                                     | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
                                     | ImGuiTableFlags_SortTristate };

    if (m_games.empty() && !m_syncer.IsRunning())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", core::tr("No games yet. Type your name and press \"Sync Games\"."));
    }

    // Room below the table for the button "Load older games".
    const float reserved{ m_moreMonths ? ImGui::GetFrameHeightWithSpacing() : 0.0f };

    if (!ImGui::BeginTable("##games", COLUMN_COUNT, flags, ImVec2(0.0f, -reserved)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    // The ID of a column is the column of the list that it sorts by. The last column (the button) cannot be sorted.
    const auto column = [](const core::SortColumn t_column) { return static_cast<ImGuiID>(t_column); };
    constexpr ImGuiTableColumnFlags fixed{ ImGuiTableColumnFlags_WidthFixed };
    constexpr ImGuiTableColumnFlags stretch{ ImGuiTableColumnFlags_WidthStretch };

    ImGui::TableSetupColumn(core::tr("Result"), fixed, RESULT_WIDTH, column(core::SortColumn::RESULT));
    ImGui::TableSetupColumn(core::tr("Color"), fixed, COLOR_WIDTH, column(core::SortColumn::COLOR));
    ImGui::TableSetupColumn(core::tr("Opponent"), stretch, 1.0f, column(core::SortColumn::OPPONENT));
    ImGui::TableSetupColumn(core::tr("Time"), fixed, TIME_WIDTH, column(core::SortColumn::TIME));
    ImGui::TableSetupColumn(core::tr("Accuracy"), fixed, ACCURACY_WIDTH, column(core::SortColumn::ACCURACY));
    ImGui::TableSetupColumn(core::tr("Moves"), fixed, MOVES_WIDTH, column(core::SortColumn::MOVES));

    // The newest games are at the top at the start.
    ImGui::TableSetupColumn(core::tr("Date"),
                            fixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending,
                            DATE_WIDTH, column(core::SortColumn::DATE));
    ImGui::TableSetupColumn(core::tr("Opening"), stretch, 1.5f, column(core::SortColumn::OPENING));
    ImGui::TableSetupColumn("", fixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoSort, ACTION_WIDTH);
    ImGui::TableHeadersRow();

    // The keys of the sorting come from the table after it was submitted. They are taken when they have changed.
    if (ImGuiTableSortSpecs* specs{ ImGui::TableGetSortSpecs() }; specs != nullptr && specs->SpecsDirty)
    {
        m_sort.clear();

        for (int i{ 0 }; i < specs->SpecsCount; ++i)
        {
            const ImGuiTableColumnSortSpecs& spec{ specs->Specs[i] };
            m_sort.push_back({ static_cast<core::SortColumn>(spec.ColumnUserID),
                               spec.SortDirection == ImGuiSortDirection_Ascending });
        }

        specs->SpecsDirty = false;
        m_visibleOutdated = true;
    }

    if (m_visibleOutdated || m_filter != m_shownFilter)
    {
        m_visible = core::filter_games(m_games, m_filter);
        core::sort_games(m_games, m_visible, m_sort);
        m_shownFilter = m_filter;
        m_visibleOutdated = false;
    }

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(m_visible.size()));

    while (clipper.Step())
    {
        for (int row{ clipper.DisplayStart }; row < clipper.DisplayEnd; ++row)
        {
            const std::size_t index{ m_visible[static_cast<std::size_t>(row)] };
            const core::GameSummary& game{ m_games[index] };

            ImGui::PushID(static_cast<int>(index));
            ImGui::TableNextRow();

            // Result: the cell is tinted, the text has the color. The reason is in the tooltip.
            ImGui::TableNextColumn();
            ImVec4 tint{ OutcomeColor(game.outcome) };
            tint.w = TINT_ALPHA;
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(tint));
            ImGui::TextColored(OutcomeColor(game.outcome), "%s", OutcomeText(game.outcome));

            if (ImGui::IsItemHovered() && !game.endReason.empty())
            {
                ImGui::SetTooltip("%s", core::describe_end_reason(game.endReason).c_str());
            }

            // The color that the user played: a disc, white or black.
            ImGui::TableNextColumn();
            {
                const ImVec2 position{ ImGui::GetCursorScreenPos() };
                const float height{ ImGui::GetTextLineHeight() };
                const ImVec2 center{ position.x + DISC_RADIUS + 2.0f, position.y + height / 2.0f };
                const bool white{ game.userColor == core::Color::WHITE };
                ImDrawList* list{ ImGui::GetWindowDrawList() };

                if (game.userColor)
                {
                    const ImU32 fill{ white ? IM_COL32(240, 240, 240, 255) : IM_COL32(20, 20, 20, 255) };
                    list->AddCircleFilled(center, DISC_RADIUS, fill);
                    list->AddCircle(center, DISC_RADIUS, IM_COL32(200, 200, 200, 255));
                }

                ImGui::Dummy(ImVec2(2.0f * DISC_RADIUS + 4.0f, height));
            }

            // The opponent, or both names if the user is not in the game.
            ImGui::TableNextColumn();

            if (game.userColor)
            {
                const bool userWhite{ *game.userColor == core::Color::WHITE };
                ImGui::Text("%s (%d)", (userWhite ? game.black : game.white).c_str(),
                            userWhite ? game.blackRating : game.whiteRating);
            }
            else
            {
                ImGui::Text("%s (%d) - %s (%d)", game.white.c_str(), game.whiteRating, game.black.c_str(),
                            game.blackRating);
            }

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(core::short_time_control(game.timeControl).c_str());

            // The accuracy of the user first, then the one of the opponent. Only chess.com has them.
            ImGui::TableNextColumn();
            const bool userBlack{ game.userColor == core::Color::BLACK };
            ImGui::TextUnformatted(
                (AccuracyText(userBlack ? game.blackAccuracy : game.whiteAccuracy) + " / "
                 + AccuracyText(userBlack ? game.whiteAccuracy : game.blackAccuracy))
                    .c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%d", game.moves);

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(core::format_game_date(game.endTime).c_str());

            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", game.eco.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(game.opening.c_str());

            if (ImGui::IsItemHovered() && !game.opening.empty())
            {
                ImGui::SetTooltip("%s", game.opening.c_str());
            }

            ImGui::TableNextColumn();

            if (ImGui::Button(core::tr("Analyze"), ImVec2(ACTION_WIDTH - 12.0f, 0.0f)))
            {
                m_analyze = index;
            }

            ImGui::PopID();
        }
    }

    ImGui::EndTable();

    // More months can be fetched. It takes a while, so it is a button and not automatic.
    if (m_moreMonths && !m_syncer.IsRunning() && ImGui::Button(core::tr("Load older games")))
    {
        m_months += MORE_MONTHS;
        StartSync(false);
    }
}
