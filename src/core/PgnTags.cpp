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
#include <string>
#include <vector>
#include "core/PgnTags.h"

namespace
{
    constexpr std::string_view UNKNOWN_PART{ "??" };
    constexpr std::string_view UNKNOWN_YEAR{ "????" };

    // The tags that describe the opening.
    constexpr std::array<std::string_view, 4> OPENING_TAGS{ "ECO", "Opening", "Variation", "SubVariation" };

    bool IsDigits(const std::string_view t_text)
    {
        return !t_text.empty() && std::ranges::all_of(t_text, [](const char t_c)
                                                      { return std::isdigit(static_cast<unsigned char>(t_c)) != 0; });
    }

    std::vector<std::string_view> Split(const std::string_view t_text)
    {
        std::vector<std::string_view> parts;
        std::size_t start{ 0 };

        while (true)
        {
            const std::size_t end{ t_text.find_first_of(".-/", start) };

            if (end == std::string_view::npos)
            {
                parts.push_back(t_text.substr(start));
                return parts;
            }

            parts.push_back(t_text.substr(start, end - start));
            start = end + 1;
        }
    }

    std::string Pad(const int t_value)
    {
        return (t_value < 10 ? "0" : "") + std::to_string(t_value);
    }

    bool IsLeapYear(const int t_year)
    {
        return (t_year % 4 == 0 && t_year % 100 != 0) || t_year % 400 == 0;
    }

    int DaysInMonth(const int t_year, const int t_month)
    {
        constexpr std::array<int, 12> DAYS{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        return t_month == 2 && IsLeapYear(t_year) ? 29 : DAYS[static_cast<std::size_t>(t_month - 1)];
    }

    bool IsUnknown(const std::string_view t_part)
    {
        return !t_part.empty() && std::ranges::all_of(t_part, [](const char t_c) { return t_c == '?'; });
    }

    std::string Trim(const std::string_view t_text)
    {
        const std::size_t first{ t_text.find_first_not_of(" \t") };

        if (first == std::string_view::npos)
        {
            return "";
        }

        return std::string{ t_text.substr(first, t_text.find_last_not_of(" \t") - first + 1) };
    }
} // namespace

//-------------------------------------------------
// Date
//-------------------------------------------------

std::optional<std::string> sg::chess::core::normalize_date(const std::string_view t_text)
{
    const std::string text{ Trim(t_text) };
    std::vector<std::string_view> parts{ Split(text) };

    if (parts.size() != 3)
    {
        return std::nullopt;
    }

    // The day first, as it is written in Germany: "24.11.2026".
    if (parts[2].size() == 4 && parts[0].size() != 4)
    {
        std::swap(parts[0], parts[2]);
    }

    const std::string_view year{ parts[0] };
    const std::string_view month{ parts[1] };
    const std::string_view day{ parts[2] };

    const bool yearKnown{ IsDigits(year) && year.size() == 4 };
    const bool monthKnown{ IsDigits(month) && month.size() <= 2 };
    const bool dayKnown{ IsDigits(day) && day.size() <= 2 };

    if (!yearKnown && !(IsUnknown(year) && year.size() == 4))
    {
        return std::nullopt;
    }

    if (!monthKnown && !IsUnknown(month))
    {
        return std::nullopt;
    }

    if (!dayKnown && !IsUnknown(day))
    {
        return std::nullopt;
    }

    const int monthValue{ monthKnown ? std::stoi(std::string{ month }) : 0 };
    const int dayValue{ dayKnown ? std::stoi(std::string{ day }) : 0 };

    if (monthKnown && (monthValue < 1 || monthValue > 12))
    {
        return std::nullopt;
    }

    if (dayKnown)
    {
        // Without a year, February may have 29 days.
        const int year4{ yearKnown ? std::stoi(std::string{ year }) : 2000 };

        if (!monthKnown || dayValue < 1 || dayValue > DaysInMonth(year4, monthValue))
        {
            return std::nullopt;
        }
    }

    const std::string monthText{ monthKnown ? Pad(monthValue) : std::string{ UNKNOWN_PART } };
    const std::string dayText{ dayKnown ? Pad(dayValue) : std::string{ UNKNOWN_PART } };

    return std::string{ yearKnown ? year : UNKNOWN_YEAR } + "." + monthText + "." + dayText;
}

//-------------------------------------------------
// Opening
//-------------------------------------------------

bool sg::chess::core::is_eco_code(const std::string_view t_text)
{
    const auto isDigit{ [](const char t_c) { return std::isdigit(static_cast<unsigned char>(t_c)) != 0; } };

    return t_text.size() == 3 && t_text[0] >= 'A' && t_text[0] <= 'E' && isDigit(t_text[1]) && isDigit(t_text[2]);
}

void sg::chess::core::set_opening_tags(PgnHeaders& t_headers, const Opening& t_opening)
{
    clear_opening_tags(t_headers);

    std::string opening{ t_opening.name };
    std::string variation;
    std::string subVariation;

    if (const std::size_t colon{ opening.find(": ") }; colon != std::string::npos)
    {
        variation = opening.substr(colon + 2);
        opening.erase(colon);

        if (const std::size_t comma{ variation.find(", ") }; comma != std::string::npos)
        {
            subVariation = variation.substr(comma + 2);
            variation.erase(comma);
        }
    }

    if (is_eco_code(t_opening.eco))
    {
        t_headers.Set("ECO", t_opening.eco);
    }

    if (!opening.empty())
    {
        t_headers.Set("Opening", opening);
    }

    if (!variation.empty())
    {
        t_headers.Set("Variation", variation);
    }

    if (!subVariation.empty())
    {
        t_headers.Set("SubVariation", subVariation);
    }
}

void sg::chess::core::clear_opening_tags(PgnHeaders& t_headers)
{
    for (const std::string_view name : OPENING_TAGS)
    {
        t_headers.Remove(name);
    }
}
