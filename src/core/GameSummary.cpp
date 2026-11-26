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
#include <cctype>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string_view>
#include <utility>
#include "core/GameSummary.h"
#include "core/I18n.h"
#include "core/TimeControl.h"

namespace
{
    // The codes of chess.com for a loss and for a draw. "win" is checked first.
    constexpr std::array<std::string_view, 7> LOSS_CODES{ "checkmated", "timeout", "resigned", "lose",
                                                          "abandoned", "kingofthehill", "threecheck" };
    constexpr std::array<std::string_view, 6> DRAW_CODES{ "agreed", "repetition", "stalemate", "insufficient",
                                                          "50move", "timevsinsufficient" };

    // Seconds of a day, for the daily games ("1/86400" is one move per day).
    constexpr int SECONDS_PER_DAY{ 86400 };

    template <std::size_t N>
    bool Contains(const std::array<std::string_view, N>& t_codes, const std::string& t_code)
    {
        for (const std::string_view code : t_codes)
        {
            if (code == t_code)
            {
                return true;
            }
        }

        return false;
    }

    // The words of the end reasons. They are marked here and translated when they are shown.
    struct ReasonText
    {
        std::string_view code;
        const char* text;
    };

    constexpr std::array<ReasonText, 12> REASONS{ {
        { "checkmated", sg::chess::core::tr_noop("Checkmate") },
        { "resigned", sg::chess::core::tr_noop("Resignation") },
        { "timeout", sg::chess::core::tr_noop("Time out") },
        { "abandoned", sg::chess::core::tr_noop("Abandoned") },
        { "agreed", sg::chess::core::tr_noop("Agreement") },
        { "repetition", sg::chess::core::tr_noop("Repetition") },
        { "stalemate", sg::chess::core::tr_noop("Stalemate") },
        { "insufficient", sg::chess::core::tr_noop("Insufficient material") },
        { "timevsinsufficient", sg::chess::core::tr_noop("Time out against insufficient material") },
        { "50move", sg::chess::core::tr_noop("50-move rule") },
        { "lose", sg::chess::core::tr_noop("Loss") },
        { "win", sg::chess::core::tr_noop("Win") },
    } };

    // Writes a number of seconds as short as possible: "30 s", "5", "15" are minutes only in a pair like 15+10.
    std::string Minutes(const int t_seconds)
    {
        if (t_seconds % 60 == 0)
        {
            return std::to_string(t_seconds / 60);
        }

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(t_seconds) / 60.0);

        return buffer;
    }
} // namespace

//-------------------------------------------------
// Results
//-------------------------------------------------

sg::chess::core::Outcome sg::chess::core::classify_result(const std::string& t_code)
{
    if (t_code == "win")
    {
        return Outcome::WIN;
    }

    if (Contains(LOSS_CODES, t_code))
    {
        return Outcome::LOSS;
    }

    if (Contains(DRAW_CODES, t_code))
    {
        return Outcome::DRAW;
    }

    return Outcome::UNKNOWN;
}

std::string sg::chess::core::pgn_result(const std::string& t_whiteCode, const std::string& t_blackCode)
{
    const Outcome white{ classify_result(t_whiteCode) };
    const Outcome black{ classify_result(t_blackCode) };

    if (white == Outcome::WIN && black != Outcome::WIN)
    {
        return "1-0";
    }

    if (black == Outcome::WIN && white != Outcome::WIN)
    {
        return "0-1";
    }

    if (white == Outcome::DRAW && black == Outcome::DRAW)
    {
        return "1/2-1/2";
    }

    return "*";
}

sg::chess::core::Outcome sg::chess::core::outcome_for(const std::string& t_result, const Color t_color)
{
    if (t_result == "1/2-1/2")
    {
        return Outcome::DRAW;
    }

    if (t_result == "1-0")
    {
        return t_color == Color::WHITE ? Outcome::WIN : Outcome::LOSS;
    }

    if (t_result == "0-1")
    {
        return t_color == Color::BLACK ? Outcome::WIN : Outcome::LOSS;
    }

    return Outcome::UNKNOWN;
}

std::string sg::chess::core::describe_end_reason(const std::string& t_code)
{
    for (const ReasonText& reason : REASONS)
    {
        if (reason.code == t_code)
        {
            return tr(reason.text);
        }
    }

    return t_code;
}

//-------------------------------------------------
// Texts
//-------------------------------------------------

std::string sg::chess::core::short_time_control(const std::string& t_timeControl)
{
    // Daily games: "1/86400" is one move in one day.
    if (t_timeControl.starts_with("1/"))
    {
        try
        {
            const int days{ std::stoi(t_timeControl.substr(2)) / SECONDS_PER_DAY };

            return days > 0 ? std::to_string(days) + " d" : t_timeControl;
        }
        catch (const std::exception&)
        {
            return t_timeControl;
        }
    }

    const auto control{ time_control_from_pgn(t_timeControl) };

    if (!control || control->baseSeconds <= 0)
    {
        return t_timeControl;
    }

    if (control->baseSeconds < 60 && control->incrementSeconds == 0)
    {
        return std::to_string(control->baseSeconds) + " s";
    }

    const std::string base{ Minutes(control->baseSeconds) };

    return control->incrementSeconds > 0 ? base + "+" + std::to_string(control->incrementSeconds) : base + " min";
}

std::string sg::chess::core::format_game_date(const std::int64_t t_seconds)
{
    if (t_seconds <= 0)
    {
        return {};
    }

    const std::chrono::sys_days day{ std::chrono::floor<std::chrono::days>(
        std::chrono::sys_seconds{ std::chrono::seconds{ t_seconds } }) };
    const std::chrono::year_month_day date{ day };

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02u", static_cast<int>(date.year()),
                  static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()));

    return buffer;
}

std::string sg::chess::core::opening_from_url(const std::string& t_url)
{
    const std::size_t slash{ t_url.find_last_of('/') };
    std::string name{ slash == std::string::npos ? t_url : t_url.substr(slash + 1) };

    // Anything after a "?" or "#" is not part of the name.
    if (const std::size_t end{ name.find_first_of("?#") }; end != std::string::npos)
    {
        name.erase(end);
    }

    for (char& c : name)
    {
        c = c == '-' ? ' ' : c;
    }

    return name;
}

int sg::chess::core::count_moves(const std::string& t_pgn)
{
    int highest{ 0 };
    int depth{ 0 };           // Inside { } comments.
    bool inTag{ false };      // Inside [ ] of a tag line: only at the start of a line.
    bool lineStart{ true };

    for (std::size_t i{ 0 }; i < t_pgn.size(); ++i)
    {
        const char c{ t_pgn[i] };

        if (depth > 0)
        {
            depth += c == '{' ? 1 : (c == '}' ? -1 : 0);
            continue;
        }

        if (inTag)
        {
            inTag = c != '\n';
            lineStart = c == '\n';
            continue;
        }

        if (c == '[' && lineStart)
        {
            inTag = true;
            continue;
        }

        if (c == '{')
        {
            depth = 1;
            continue;
        }

        lineStart = c == '\n';

        const bool boundary{ i == 0 || std::isspace(static_cast<unsigned char>(t_pgn[i - 1])) != 0
                             || t_pgn[i - 1] == '}' };

        if (boundary && std::isdigit(static_cast<unsigned char>(c)) != 0)
        {
            std::size_t end{ i };
            int number{ 0 };

            while (end < t_pgn.size() && std::isdigit(static_cast<unsigned char>(t_pgn[end])) != 0 && number < 10000)
            {
                number = number * 10 + (t_pgn[end] - '0');
                ++end;
            }

            // A move number ends with a dot. A result like "1-0" or "1/2-1/2" does not.
            if (end < t_pgn.size() && t_pgn[end] == '.')
            {
                highest = std::max(highest, number);
            }

            i = end - 1;
        }
    }

    return highest;
}
