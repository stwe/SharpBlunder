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

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "core/GameSummary.h"

namespace sg::chess::core
{
    //-------------------------------------------------
    // GameFilter
    //-------------------------------------------------

    /**
     * @brief What the user has chosen to see on the list of games.
     */
    struct GameFilter
    {
        std::string timeClass; ///< "bullet", "blitz", "rapid" or "daily". Empty: all
        std::string text; ///< Part of a name, of the opening or of the ECO code. Upper and lower case do not matter

        /**
         * @brief Compares two filters.
         *
         * @return True if both show the same games.
         */
        [[nodiscard]] bool operator==(const GameFilter&) const = default;
    };

    //-------------------------------------------------
    // Sorting
    //-------------------------------------------------

    /**
     * @brief The columns that the list of games can be sorted by.
     */
    enum class SortColumn : std::uint8_t
    {
        RESULT, ///< Win, draw, loss (from the point of view of the user). Ascending: wins first
        COLOR, ///< The color that the user played. Ascending: White first
        OPPONENT, ///< The name of the opponent, upper and lower case do not matter
        TIME, ///< The time control, from short to long. A game with an increment counts as longer
        ACCURACY, ///< The accuracy of the user. Games without it are always last
        MOVES, ///< The number of moves
        DATE, ///< When the game ended
        OPENING ///< The name of the opening. Games without one are always last
    };

    /**
     * @brief One column to sort by, and the direction.
     */
    struct SortKey
    {
        SortColumn column{ SortColumn::DATE }; ///< The column
        bool ascending{ false }; ///< True: from small to large (A to Z, old to new). False: the other way

        /**
         * @brief Compares two keys.
         *
         * @return True if both have the same column and direction.
         */
        [[nodiscard]] bool operator==(const SortKey&) const = default;
    };

    /**
     * @brief Sorts a list of indexes of games.
     *
     * The first key is the most important one. Games that are equal in all keys keep the usual order: the newest
     * first. Without keys the list gets this usual order.
     *
     * @param t_games The games.
     * @param t_indexes The indexes in t_games to sort. Every index must be valid.
     * @param t_keys The columns to sort by, the most important first.
     */
    void sort_games(const std::vector<GameSummary>& t_games, std::vector<std::size_t>& t_indexes,
                    const std::vector<SortKey>& t_keys);

    /**
     * @brief Estimates how long a game with a time control lasts, in seconds.
     *
     * The base time plus 40 times the increment: the usual way to compare time controls with an increment. A daily
     * game ("1/86400") counts as its days. A time control that cannot be read is the largest value.
     *
     * @param t_timeControl The PGN value of the time control, e.g. "900+10".
     *
     * @return The duration in seconds.
     */
    [[nodiscard]] long estimated_duration(const std::string& t_timeControl);

    //-------------------------------------------------
    // List
    //-------------------------------------------------

    /**
     * @brief Sorts games: the newest first. Games that ended at the same time keep a fixed order (by id).
     *
     * @param t_games The games.
     */
    void sort_newest_first(std::vector<GameSummary>& t_games);

    /**
     * @brief Adds games to a list. A game that is already in the list (same source and id) is not added again,
     *        and the new data replaces the old: a game can change, e.g. when accuracies come later.
     *        The list is sorted afterwards, the newest first.
     *
     * @param t_list The list.
     * @param t_new The games to add.
     */
    void add_games(std::vector<GameSummary>& t_list, std::vector<GameSummary> t_new);

    /**
     * @brief Tells whether a game is shown with a filter.
     *
     * @param t_game The game.
     * @param t_filter The filter.
     *
     * @return True if the game has the time class (if there is one) and the text is in a name, the opening or the
     *         ECO code (if there is a text).
     */
    [[nodiscard]] bool matches(const GameSummary& t_game, const GameFilter& t_filter);

    /**
     * @brief Finds the games that a filter shows.
     *
     * @param t_games The games.
     * @param t_filter The filter.
     *
     * @return The indexes of the games in t_games, in the order of t_games.
     */
    [[nodiscard]] std::vector<std::size_t> filter_games(const std::vector<GameSummary>& t_games,
                                                        const GameFilter& t_filter);

    /**
     * @brief Finds the time classes that there are games of, in the usual order: bullet, blitz, rapid, daily.
     *
     * @param t_games The games.
     *
     * @return The classes. Games with another or no class are not counted.
     */
    [[nodiscard]] std::vector<std::string> time_classes(const std::vector<GameSummary>& t_games);
} // namespace sg::chess::core
