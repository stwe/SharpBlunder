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
#include <string_view>
#include <utility>
#include "core/GameList.h"

namespace
{
    // The time classes of chess.com, from the fastest to the slowest.
    constexpr std::array<std::string_view, 4> TIME_CLASSES{ "bullet", "blitz", "rapid", "daily" };

    std::string Lower(std::string t_text)
    {
        std::ranges::transform(t_text, t_text.begin(),
                               [](const unsigned char t_c) { return static_cast<char>(std::tolower(t_c)); });

        return t_text;
    }

    bool Has(const std::string& t_text, const std::string& t_lowerPart)
    {
        return Lower(t_text).find(t_lowerPart) != std::string::npos;
    }
} // namespace

void sg::chess::core::sort_newest_first(std::vector<GameSummary>& t_games)
{
    std::ranges::stable_sort(t_games,
                             [](const GameSummary& t_a, const GameSummary& t_b)
                             {
                                 if (t_a.endTime != t_b.endTime)
                                 {
                                     return t_a.endTime > t_b.endTime;
                                 }

                                 return t_a.id < t_b.id;
                             });
}

void sg::chess::core::add_games(std::vector<GameSummary>& t_list, std::vector<GameSummary> t_new)
{
    for (GameSummary& game : t_new)
    {
        const auto same{ std::ranges::find_if(t_list, [&](const GameSummary& t_other)
                                               { return t_other.source == game.source && t_other.id == game.id; }) };

        if (same != t_list.end())
        {
            *same = std::move(game);
        }
        else
        {
            t_list.push_back(std::move(game));
        }
    }

    sort_newest_first(t_list);
}

bool sg::chess::core::matches(const GameSummary& t_game, const GameFilter& t_filter)
{
    if (!t_filter.timeClass.empty() && t_game.timeClass != t_filter.timeClass)
    {
        return false;
    }

    if (t_filter.text.empty())
    {
        return true;
    }

    const std::string part{ Lower(t_filter.text) };

    return Has(t_game.white, part) || Has(t_game.black, part) || Has(t_game.opening, part) || Has(t_game.eco, part);
}

std::vector<std::size_t> sg::chess::core::filter_games(const std::vector<GameSummary>& t_games,
                                                        const GameFilter& t_filter)
{
    std::vector<std::size_t> found;

    for (std::size_t i{ 0 }; i < t_games.size(); ++i)
    {
        if (matches(t_games[i], t_filter))
        {
            found.push_back(i);
        }
    }

    return found;
}

std::vector<std::string> sg::chess::core::time_classes(const std::vector<GameSummary>& t_games)
{
    std::vector<std::string> classes;

    for (const std::string_view name : TIME_CLASSES)
    {
        if (std::ranges::any_of(t_games, [&](const GameSummary& t_game) { return t_game.timeClass == name; }))
        {
            classes.emplace_back(name);
        }
    }

    return classes;
}
