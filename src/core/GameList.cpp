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
#include <exception>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>
#include "core/GameList.h"
#include "core/TimeControl.h"

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

    // The increment counts like this many moves of the game in the estimated duration (the usual value is 40).
    constexpr long INCREMENT_MOVES{ 40 };

    // Compares two values: -1 if a is first, 1 if b is first, 0 if they are the same.
    template <typename T>
    int Order(const T& t_a, const T& t_b)
    {
        return t_a < t_b ? -1 : (t_b < t_a ? 1 : 0);
    }

    // The name of the other side, in lower case. If the user was not in the game: both names.
    std::string OpponentKey(const sg::chess::core::GameSummary& t_game)
    {
        using sg::chess::core::Color;

        if (!t_game.userColor)
        {
            return Lower(t_game.white + " " + t_game.black);
        }

        return Lower(*t_game.userColor == Color::WHITE ? t_game.black : t_game.white);
    }

    std::optional<double> UserAccuracy(const sg::chess::core::GameSummary& t_game)
    {
        return t_game.userColor == sg::chess::core::Color::BLACK ? t_game.blackAccuracy : t_game.whiteAccuracy;
    }

    // Result: wins, draws, losses, and games without a result last.
    int ResultRank(const sg::chess::core::GameSummary& t_game)
    {
        using sg::chess::core::Outcome;

        switch (t_game.outcome)
        {
        case Outcome::WIN:
            return 0;
        case Outcome::DRAW:
            return 1;
        case Outcome::LOSS:
            return 2;
        default:
            return 3;
        }
    }

    // What a column says about two games, from small to large. Missing values come last, whatever the direction is:
    // t_ascending is only used to put them there.
    int CompareBy(const sg::chess::core::GameSummary& t_a, const sg::chess::core::GameSummary& t_b,
                  const sg::chess::core::SortKey& t_key)
    {
        using sg::chess::core::Color;
        using sg::chess::core::SortColumn;

        // Missing values: both missing is a tie, one missing is last.
        const auto withMissing = [&](const bool t_aMissing, const bool t_bMissing, const int t_order)
        {
            if (t_aMissing || t_bMissing)
            {
                return t_aMissing == t_bMissing ? 0 : (t_aMissing ? 1 : -1);
            }

            return t_key.ascending ? t_order : -t_order;
        };

        int order{ 0 };

        switch (t_key.column)
        {
        case SortColumn::RESULT:
            order = Order(ResultRank(t_a), ResultRank(t_b));
            break;
        case SortColumn::COLOR:
            // Games without a user color are last in the same way as a black game: they are not "White".
            order = Order(t_a.userColor == Color::WHITE ? 0 : 1, t_b.userColor == Color::WHITE ? 0 : 1);
            break;
        case SortColumn::OPPONENT:
            order = Order(OpponentKey(t_a), OpponentKey(t_b));
            break;
        case SortColumn::TIME:
            order = Order(sg::chess::core::estimated_duration(t_a.timeControl),
                          sg::chess::core::estimated_duration(t_b.timeControl));
            break;
        case SortColumn::ACCURACY:
            return withMissing(!UserAccuracy(t_a), !UserAccuracy(t_b),
                               Order(UserAccuracy(t_a).value_or(0.0), UserAccuracy(t_b).value_or(0.0)));
        case SortColumn::MOVES:
            order = Order(t_a.moves, t_b.moves);
            break;
        case SortColumn::DATE:
            order = Order(t_a.endTime, t_b.endTime);
            break;
        case SortColumn::OPENING:
            return withMissing(t_a.opening.empty(), t_b.opening.empty(),
                               Order(Lower(t_a.opening), Lower(t_b.opening)));
        }

        return t_key.ascending ? order : -order;
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

long sg::chess::core::estimated_duration(const std::string& t_timeControl)
{
    // Daily games: "1/86400" is one move in one day.
    if (t_timeControl.starts_with("1/"))
    {
        try
        {
            return std::stol(t_timeControl.substr(2));
        }
        catch (const std::exception&)
        {
            return std::numeric_limits<long>::max();
        }
    }

    const auto control{ time_control_from_pgn(t_timeControl) };

    if (!control)
    {
        return std::numeric_limits<long>::max();
    }

    return control->baseSeconds + INCREMENT_MOVES * control->incrementSeconds;
}

void sg::chess::core::sort_games(const std::vector<GameSummary>& t_games, std::vector<std::size_t>& t_indexes,
                                 const std::vector<SortKey>& t_keys)
{
    std::ranges::stable_sort(t_indexes,
                             [&](const std::size_t t_a, const std::size_t t_b)
                             {
                                 const GameSummary& a{ t_games[t_a] };
                                 const GameSummary& b{ t_games[t_b] };

                                 for (const SortKey& key : t_keys)
                                 {
                                     if (const int order{ CompareBy(a, b, key) }; order != 0)
                                     {
                                         return order < 0;
                                     }
                                 }

                                 // Equal in all keys: the usual order, the newest first.
                                 if (a.endTime != b.endTime)
                                 {
                                     return a.endTime > b.endTime;
                                 }

                                 return a.id < b.id;
                             });
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
