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
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include "core/TimeControl.h"

namespace
{
    using sg::chess::core::TimeControl;

    constexpr double SECONDS_PER_MINUTE{ 60.0 };
    constexpr double SECONDS_PER_HOUR{ 3600.0 };

    // A time longer than this (about 28 hours) is a typing error, and keeps the numbers small.
    constexpr double MAX_SECONDS{ 100000.0 };

    // The units that are understood, in lower case, and their size in seconds.
    constexpr std::array<std::pair<std::string_view, double>, 17> UNITS{
        { { "s", 1.0 }, { "sec", 1.0 }, { "secs", 1.0 }, { "second", 1.0 }, { "seconds", 1.0 },
          { "m", SECONDS_PER_MINUTE }, { "min", SECONDS_PER_MINUTE }, { "mins", SECONDS_PER_MINUTE },
          { "minute", SECONDS_PER_MINUTE }, { "minutes", SECONDS_PER_MINUTE },
          { "h", SECONDS_PER_HOUR }, { "hr", SECONDS_PER_HOUR }, { "hrs", SECONDS_PER_HOUR },
          { "hour", SECONDS_PER_HOUR }, { "hours", SECONDS_PER_HOUR },
          { "sek", 1.0 }, { "std", SECONDS_PER_HOUR } }
    };

    bool IsSpace(const char t_c)
    {
        return std::isspace(static_cast<unsigned char>(t_c)) != 0;
    }

    bool IsDigit(const char t_c)
    {
        return std::isdigit(static_cast<unsigned char>(t_c)) != 0;
    }

    char Lower(const char t_c)
    {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(t_c)));
    }

    void SkipSpaces(const std::string_view t_text, std::size_t& t_pos)
    {
        while (t_pos < t_text.size() && IsSpace(t_text[t_pos]))
        {
            ++t_pos;
        }
    }

    // Reads one or more amounts like "1h 30min" and adds them up. The unit of an amount that has none is
    // t_defaultUnit. Stops at the first character that does not belong to an amount (the "+" or the end).
    std::optional<double> ReadTime(const std::string_view t_text, std::size_t& t_pos, const double t_defaultUnit)
    {
        double total{ 0.0 };
        bool any{ false };

        while (true)
        {
            SkipSpaces(t_text, t_pos);

            const std::size_t start{ t_pos };
            while (t_pos < t_text.size() && (IsDigit(t_text[t_pos]) || t_text[t_pos] == '.' || t_text[t_pos] == ','))
            {
                ++t_pos;
            }

            if (start == t_pos)
            {
                break;
            }

            std::string number{ t_text.substr(start, t_pos - start) };
            std::ranges::replace(number, ',', '.');

            char* end{ nullptr };
            const double value{ std::strtod(number.c_str(), &end) };
            if (end != number.c_str() + number.size())
            {
                return std::nullopt;
            }

            SkipSpaces(t_text, t_pos);

            std::string unit;
            while (t_pos < t_text.size() && std::isalpha(static_cast<unsigned char>(t_text[t_pos])) != 0)
            {
                unit += Lower(t_text[t_pos]);
                ++t_pos;
            }

            // Only the first amount may go without a unit: "15 10" is not "15 minutes and 10 minutes".
            if (unit.empty() && any)
            {
                return std::nullopt;
            }

            double factor{ t_defaultUnit };
            if (!unit.empty())
            {
                const auto it{ std::ranges::find(UNITS, unit, &std::pair<std::string_view, double>::first) };
                if (it == UNITS.end())
                {
                    return std::nullopt;
                }

                factor = it->second;
            }

            total += value * factor;
            any = true;

            if (total > MAX_SECONDS)
            {
                return std::nullopt;
            }
        }

        return any ? std::optional<double>{ total } : std::nullopt;
    }

    // "12" for 12 seconds. Whole numbers only.
    std::optional<int> ReadWholeSeconds(const std::string_view t_text)
    {
        if (t_text.empty() || t_text.size() > 6 || !std::ranges::all_of(t_text, IsDigit))
        {
            return std::nullopt;
        }

        return std::atoi(std::string{ t_text }.c_str());
    }

    // One period of a PGN time control: moves/seconds, seconds, seconds+increment, or *seconds.
    bool IsPeriod(const std::string_view t_period, const bool t_last)
    {
        if (const std::size_t slash{ t_period.find('/') }; slash != std::string_view::npos)
        {
            return ReadWholeSeconds(t_period.substr(0, slash)) && ReadWholeSeconds(t_period.substr(slash + 1));
        }

        if (t_period.starts_with('*'))
        {
            return t_last && ReadWholeSeconds(t_period.substr(1));
        }

        if (const std::size_t plus{ t_period.find('+') }; plus != std::string_view::npos)
        {
            return t_last && ReadWholeSeconds(t_period.substr(0, plus)) && ReadWholeSeconds(t_period.substr(plus + 1));
        }

        return ReadWholeSeconds(t_period).has_value();
    }

    std::string Amount(const int t_seconds)
    {
        const int hours{ t_seconds / 3600 };
        const int minutes{ t_seconds % 3600 / 60 };
        const int seconds{ t_seconds % 60 };

        if (t_seconds < 60 || (t_seconds < 3600 && seconds != 0))
        {
            return std::to_string(t_seconds) + " s";
        }

        std::string text;
        if (hours > 0)
        {
            text += std::to_string(hours) + " h";
        }

        if (minutes > 0)
        {
            text += (text.empty() ? "" : " ") + std::to_string(minutes) + " min";
        }

        if (seconds > 0)
        {
            text += " " + std::to_string(seconds) + " s";
        }

        return text;
    }
} // namespace

//-------------------------------------------------
// Input and output
//-------------------------------------------------

std::optional<sg::chess::core::TimeControl> sg::chess::core::parse_time_control(const std::string_view t_text)
{
    std::size_t pos{ 0 };

    const auto base{ ReadTime(t_text, pos, SECONDS_PER_MINUTE) };
    if (!base)
    {
        return std::nullopt;
    }

    double increment{ 0.0 };

    SkipSpaces(t_text, pos);
    if (pos < t_text.size())
    {
        if (t_text[pos] != '+' && t_text[pos] != '|')
        {
            return std::nullopt;
        }

        ++pos;

        const auto added{ ReadTime(t_text, pos, 1.0) };
        if (!added)
        {
            return std::nullopt;
        }

        increment = *added;

        SkipSpaces(t_text, pos);
        if (pos != t_text.size())
        {
            return std::nullopt;
        }
    }

    const TimeControl control{ static_cast<int>(std::lround(*base)), static_cast<int>(std::lround(increment)) };

    // "0" says nothing, and "0+5" is fine (a game that is played on the increment alone).
    if (control.baseSeconds == 0 && control.incrementSeconds == 0)
    {
        return std::nullopt;
    }

    return control;
}

std::string sg::chess::core::time_control_to_pgn(const TimeControl& t_control)
{
    std::string value{ std::to_string(t_control.baseSeconds) };

    if (t_control.incrementSeconds > 0)
    {
        value += "+" + std::to_string(t_control.incrementSeconds);
    }

    return value;
}

std::optional<sg::chess::core::TimeControl> sg::chess::core::time_control_from_pgn(const std::string_view t_value)
{
    if (!is_pgn_time_control(t_value) || t_value.find_first_of("/:*-?") != std::string_view::npos)
    {
        return std::nullopt;
    }

    const std::size_t plus{ t_value.find('+') };

    TimeControl control;
    control.baseSeconds = *ReadWholeSeconds(t_value.substr(0, plus));

    if (plus != std::string_view::npos)
    {
        control.incrementSeconds = *ReadWholeSeconds(t_value.substr(plus + 1));
    }

    return control;
}

std::string sg::chess::core::describe_time_control(const TimeControl& t_control)
{
    std::string text{ Amount(t_control.baseSeconds) };

    if (t_control.incrementSeconds > 0)
    {
        text += " + " + std::to_string(t_control.incrementSeconds) + " s";
    }

    return text;
}

bool sg::chess::core::is_pgn_time_control(const std::string_view t_value)
{
    if (t_value == "?" || t_value == "-")
    {
        return true;
    }

    std::size_t start{ 0 };

    while (true)
    {
        const std::size_t colon{ t_value.find(':', start) };
        const bool last{ colon == std::string_view::npos };

        if (!IsPeriod(t_value.substr(start, last ? std::string_view::npos : colon - start), last))
        {
            return false;
        }

        if (last)
        {
            return true;
        }

        start = colon + 1;
    }
}

std::optional<std::string> sg::chess::core::time_control_from_input(const std::string_view t_text)
{
    if (const auto control{ parse_time_control(t_text) })
    {
        return time_control_to_pgn(*control);
    }

    // Spaces around a correct tag value do not matter.
    const std::size_t first{ t_text.find_first_not_of(" \t") };
    if (first == std::string_view::npos)
    {
        return std::nullopt;
    }

    const std::string_view trimmed{ t_text.substr(first, t_text.find_last_not_of(" \t") - first + 1) };

    // A plain "600" or "600+5" is not taken: the text was not understood as a time, and it would be a time of a
    // different size than the person may think. Only what cannot be a simple time control is kept as it is.
    const bool special{ trimmed == "?" || trimmed == "-" || trimmed.find_first_of("/:*") != std::string_view::npos };

    if (special && is_pgn_time_control(trimmed))
    {
        return std::string{ trimmed };
    }

    return std::nullopt;
}

std::string sg::chess::core::time_control_for_input(const std::string_view t_value)
{
    if (const auto control{ time_control_from_pgn(t_value) })
    {
        return describe_time_control(*control);
    }

    return std::string{ t_value };
}
