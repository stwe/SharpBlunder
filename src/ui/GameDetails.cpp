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
#include <string>
#include <string_view>
#include "core/PgnTags.h"
#include "core/TimeControl.h"
#include "ui/GameDetails.h"

namespace
{
    constexpr const char* POPUP{ "Game details" };
    constexpr float LABEL_WIDTH{ 110.0f };
    constexpr float FIELD_WIDTH{ 380.0f };
    constexpr float RATING_FIELD_WIDTH{ 80.0f };
    constexpr float PRESET_WIDTH{ 100.0f };
    constexpr ImVec4 ERROR_COLOR{ 1.0f, 0.4f, 0.4f, 1.0f };

    // The time controls that people use, for the list next to the field. The text is what the field takes.
    struct TimeControlPreset
    {
        const char* text;
        const char* label;
    };

    constexpr std::array<TimeControlPreset, 10> PRESETS{ { { "1+0", "1 min  (Bullet)" },
                                                           { "2+1", "2 min + 1 s  (Bullet)" },
                                                           { "3+0", "3 min  (Blitz)" },
                                                           { "3+2", "3 min + 2 s  (Blitz)" },
                                                           { "5+0", "5 min  (Blitz)" },
                                                           { "5+3", "5 min + 3 s  (Blitz)" },
                                                           { "10+0", "10 min  (Rapid)" },
                                                           { "15+10", "15 min + 10 s  (Rapid)" },
                                                           { "30+0", "30 min  (Classical)" },
                                                           { "90+30", "90 min + 30 s  (Classical)" } } };

    // What a result looks like in the PGN file, and what the list in the window says.
    struct ResultOption
    {
        const char* value;
        const char* label;
    };

    constexpr std::array<ResultOption, 4> RESULTS{ { { "*", "* (unknown, or the game is not finished)" },
                                                     { "1-0", "1-0 (White won)" },
                                                     { "0-1", "0-1 (Black won)" },
                                                     { "1/2-1/2", "1/2-1/2 (draw)" } } };

    // PGN writes "?" and "????.??.??" for what is not known. The window shows an empty field instead.
    std::string Shown(const std::string& t_value)
    {
        return t_value == "?" || t_value == "????.??.??" ? "" : t_value;
    }

    template <std::size_t N>
    void Fill(std::array<char, N>& t_field, const std::string& t_text)
    {
        const std::size_t length{ std::min(t_text.size(), N - 1) };

        std::memcpy(t_field.data(), t_text.data(), length);
        t_field[length] = '\0';
    }

    // The text of a field, without spaces at its ends.
    template <std::size_t N>
    std::string Text(const std::array<char, N>& t_field)
    {
        const std::string_view text{ t_field.data() };
        const std::size_t first{ text.find_first_not_of(" \t\r\n") };

        if (first == std::string_view::npos)
        {
            return "";
        }

        return std::string{ text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1) };
    }

    // A tag that every game has: it is "?" if the field is empty.
    void SetRoster(sg::chess::core::PgnHeaders& t_headers, const std::string_view t_name, const std::string& t_text,
                   const std::string_view t_unknown)
    {
        t_headers.Set(t_name, t_text.empty() ? t_unknown : std::string_view{ t_text });
    }

    // A tag that can be left out: it is removed if the field is empty.
    void SetOptional(sg::chess::core::PgnHeaders& t_headers, const std::string_view t_name, const std::string& t_text)
    {
        if (t_text.empty())
        {
            t_headers.Remove(t_name);
        }
        else
        {
            t_headers.Set(t_name, t_text);
        }
    }

    // A row of the table: the label, and a field that the caller builds.
    template <typename Builder>
    void Row(const char* t_label, Builder t_build)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(t_label);
        ImGui::TableSetColumnIndex(1);
        t_build();
    }
} // namespace

//-------------------------------------------------
// Control
//-------------------------------------------------

void sg::chess::ui::GameDetails::Open(const core::PgnHeaders& t_headers)
{
    Fill(m_white, Shown(t_headers.Get("White")));
    Fill(m_whiteRating, t_headers.PlayerRating(core::Color::WHITE));
    Fill(m_black, Shown(t_headers.Get("Black")));
    Fill(m_blackRating, t_headers.PlayerRating(core::Color::BLACK));
    Fill(m_event, Shown(t_headers.Get("Event")));
    Fill(m_site, Shown(t_headers.Get("Site")));
    Fill(m_round, Shown(t_headers.Get("Round")));
    Fill(m_date, Shown(t_headers.Get("Date")));
    Fill(m_timeControl, core::time_control_for_input(Shown(t_headers.Get("TimeControl"))));
    Fill(m_termination, Shown(t_headers.Get("Termination")));

    const std::string result{ t_headers.Get("Result") };
    m_result = 0;

    for (std::size_t i{ 0 }; i < RESULTS.size(); ++i)
    {
        if (result == RESULTS[i].value)
        {
            m_result = static_cast<int>(i);
        }
    }

    m_wantOpen = true;
}

//-------------------------------------------------
// Frame steps
//-------------------------------------------------

bool sg::chess::ui::GameDetails::Draw(core::PgnHeaders& t_headers)
{
    bool saved{ false };

    // The window is opened from here, where it is built every frame.
    if (m_wantOpen)
    {
        ImGui::OpenPopup(POPUP);
        m_wantOpen = false;
    }

    const ImVec2 displaySize{ ImGui::GetIO().DisplaySize };
    ImGui::SetNextWindowPos(ImVec2(displaySize.x / 2.0f, displaySize.y / 2.0f), ImGuiCond_Appearing,
                            ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal(POPUP, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
    {
        return false;
    }

    if (ImGui::BeginTable("##fields", 2, ImGuiTableFlags_NoSavedSettings))
    {
        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, LABEL_WIDTH);
        ImGui::TableSetupColumn("##field", ImGuiTableColumnFlags_WidthFixed, FIELD_WIDTH);

        const float nameWidth{ FIELD_WIDTH - RATING_FIELD_WIDTH - ImGui::GetStyle().ItemSpacing.x };

        Row("White", [&]
        {
            ImGui::SetNextItemWidth(nameWidth);
            ImGui::InputTextWithHint("##white", "Name", m_white.data(), m_white.size());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(RATING_FIELD_WIDTH);
            ImGui::InputTextWithHint("##whiteElo", "Rating", m_whiteRating.data(), m_whiteRating.size(),
                                     ImGuiInputTextFlags_CharsDecimal);
        });

        Row("Black", [&]
        {
            ImGui::SetNextItemWidth(nameWidth);
            ImGui::InputTextWithHint("##black", "Name", m_black.data(), m_black.size());
            ImGui::SameLine();
            ImGui::SetNextItemWidth(RATING_FIELD_WIDTH);
            ImGui::InputTextWithHint("##blackElo", "Rating", m_blackRating.data(), m_blackRating.size(),
                                     ImGuiInputTextFlags_CharsDecimal);
        });

        Row("Result", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH);
            if (ImGui::BeginCombo("##result", RESULTS[static_cast<std::size_t>(m_result)].label))
            {
                for (std::size_t i{ 0 }; i < RESULTS.size(); ++i)
                {
                    if (ImGui::Selectable(RESULTS[i].label, static_cast<int>(i) == m_result))
                    {
                        m_result = static_cast<int>(i);
                    }
                }

                ImGui::EndCombo();
            }
        });

        Row("Event", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH);
            ImGui::InputTextWithHint("##event", "e.g. a tournament", m_event.data(), m_event.size());
        });

        Row("Place", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH);
            ImGui::InputTextWithHint("##site", "e.g. a city, or a website", m_site.data(), m_site.size());
        });

        Row("Round", [&]
        {
            ImGui::SetNextItemWidth(RATING_FIELD_WIDTH);
            ImGui::InputText("##round", m_round.data(), m_round.size());
        });

        Row("Date", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH);
            ImGui::InputTextWithHint("##date", "YYYY.MM.DD, e.g. 2026.11.24", m_date.data(), m_date.size(),
                                     ImGuiInputTextFlags_AutoSelectAll);

            const std::string date{ Text(m_date) };
            if (!date.empty() && !core::normalize_date(date))
            {
                ImGui::TextColored(ERROR_COLOR, "Not a date. Use year.month.day, e.g. 2026.11.24");
            }
        });

        Row("Time control", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH - PRESET_WIDTH - ImGui::GetStyle().ItemSpacing.x);
            ImGui::InputTextWithHint("##time", "e.g. 15+10 or 5 min + 3 s", m_timeControl.data(),
                                     m_timeControl.size(), ImGuiInputTextFlags_AutoSelectAll);

            ImGui::SameLine();
            ImGui::SetNextItemWidth(PRESET_WIDTH);
            if (ImGui::BeginCombo("##presets", "Presets"))
            {
                for (const TimeControlPreset& preset : PRESETS)
                {
                    if (ImGui::Selectable(preset.label))
                    {
                        Fill(m_timeControl, core::time_control_for_input(*core::time_control_from_input(preset.text)));
                    }
                }

                ImGui::EndCombo();
            }

            // One line below the field says how the text is understood, so that no time is a surprise.
            const std::string text{ Text(m_timeControl) };
            if (text.empty())
            {
                ImGui::TextDisabled("15+10 means 15 minutes plus 10 seconds per move");
            }
            else if (const auto value{ core::time_control_from_input(text) })
            {
                const auto control{ core::time_control_from_pgn(*value) };
                ImGui::TextDisabled("Saved as %s%s%s%s", value->c_str(), control ? " (" : "",
                                    control ? core::describe_time_control(*control).c_str() : "", control ? ")" : "");
            }
            else
            {
                ImGui::TextColored(ERROR_COLOR, "Not understood. Try 15+10, 5 min + 3 s or 90 s");
            }
        });

        Row("Opening", [&]
        {
            // The opening is set from the moves. It is shown here so that it is clear what the file will say.
            const std::string eco{ t_headers.Get("ECO") };
            std::string opening{ t_headers.Get("Opening") };
            for (const char* name : { "Variation", "SubVariation" })
            {
                if (const std::string part{ t_headers.Get(name) }; !part.empty())
                {
                    opening += (opening.empty() ? "" : ": ") + part;
                }
            }

            const std::string text{ (eco.empty() ? "" : eco + " ") + opening };
            ImGui::TextDisabled("%s", text.empty() ? "not known" : text.c_str());
        });

        Row("Termination", [&]
        {
            ImGui::SetNextItemWidth(FIELD_WIDTH);
            ImGui::InputTextWithHint("##termination", "how it ended, e.g. White won by resignation",
                                     m_termination.data(), m_termination.size());
        });

        ImGui::EndTable();
    }

    ImGui::Spacing();

    const std::string dateText{ Text(m_date) };
    const std::string timeText{ Text(m_timeControl) };
    const std::optional<std::string> date{ dateText.empty() ? std::optional<std::string>{ "????.??.??" }
                                                           : core::normalize_date(dateText) };
    const std::optional<std::string> timeControl{ timeText.empty() ? std::optional<std::string>{ "" }
                                                                   : core::time_control_from_input(timeText) };

    // A date or a time that is not understood is not written to the file.
    ImGui::BeginDisabled(!date || !timeControl);
    if (ImGui::Button("Save"))
    {
        SetRoster(t_headers, "White", Text(m_white), "?");
        SetRoster(t_headers, "Black", Text(m_black), "?");
        SetRoster(t_headers, "Event", Text(m_event), "?");
        SetRoster(t_headers, "Site", Text(m_site), "?");
        SetRoster(t_headers, "Round", Text(m_round), "?");
        t_headers.Set("Date", *date);
        t_headers.Set("Result", RESULTS[static_cast<std::size_t>(m_result)].value);

        SetOptional(t_headers, "WhiteElo", Text(m_whiteRating));
        SetOptional(t_headers, "BlackElo", Text(m_blackRating));
        SetOptional(t_headers, "TimeControl", *timeControl);
        SetOptional(t_headers, "Termination", Text(m_termination));

        saved = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return saved;
}
