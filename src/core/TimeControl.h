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

#include <optional>
#include <string>
#include <string_view>

namespace sg::chess::core
{
    //-------------------------------------------------
    // TimeControl
    //-------------------------------------------------

    /**
     * @brief A simple time control: some time for the whole game, and optionally some time added after every move.
     *
     * More complicated controls (e.g. 40 moves in 2 hours, then 1 hour) are not represented here. PGN can say them
     * with the tag TimeControl, and this program keeps such a tag as it is.
     */
    struct TimeControl
    {
        int baseSeconds{ 0 };       ///< The time for each player at the start, in seconds
        int incrementSeconds{ 0 };  ///< The time added after every move, in seconds

        [[nodiscard]] bool operator==(const TimeControl&) const = default;
    };

    //-------------------------------------------------
    // Input and output
    //-------------------------------------------------

    /**
     * @brief Reads a time control as a person writes it, e.g. "15+10" or "15 min + 10 s".
     *
     * The text has the time at the start, and may have the time added per move after a "+" or a "|". Times are
     * written as a number with a unit: s, sec, second(s) for seconds, m, min, minute(s) for minutes, h, hour(s) for
     * hours. A number may have a decimal point or comma ("1,5 min"). Several amounts are added ("1h 30min"), and all
     * of them but the first must have a unit.
     * Without a unit the time at the start is in minutes, and the time added per move is in seconds. That is how
     * chess players say it: "15+10" is 15 minutes and 10 seconds per move. Capital letters and spaces do not matter.
     *
     * @param t_text The text.
     *
     * @return The time control, or std::nullopt if the text is not understood, is empty, or says no time at all.
     */
    [[nodiscard]] std::optional<TimeControl> parse_time_control(std::string_view t_text);

    /**
     * @brief Writes a time control as the value of the PGN tag TimeControl: seconds only.
     *
     * @param t_control The time control.
     *
     * @return For example "900+10", or "300" if nothing is added per move.
     */
    [[nodiscard]] std::string time_control_to_pgn(const TimeControl& t_control);

    /**
     * @brief Reads the value of a PGN tag TimeControl, if it is a simple one (seconds, optionally "+" seconds).
     *
     * @param t_value The value of the tag, e.g. "900+10".
     *
     * @return The time control, or std::nullopt for "?", "-", and for controls with several periods.
     */
    [[nodiscard]] std::optional<TimeControl> time_control_from_pgn(std::string_view t_value);

    /**
     * @brief Writes a time control in words, for people.
     *
     * @param t_control The time control.
     *
     * @return For example "15 min + 10 s", "90 s" or "1 h 30 min".
     */
    [[nodiscard]] std::string describe_time_control(const TimeControl& t_control);

    /**
     * @brief Checks whether a text is a correct value of the PGN tag TimeControl.
     *
     * Allowed are "?" and "-" (not known, no time control), and periods separated by ":": moves per seconds
     * ("40/7200"), seconds ("300"), seconds plus an increment ("300+5", last period only) and an hourglass ("*60", last
     * period only).
     *
     * @param t_value The text.
     *
     * @return true if it is correct.
     */
    [[nodiscard]] bool is_pgn_time_control(std::string_view t_value);

    /**
     * @brief Turns what a person has typed into the value for the PGN tag.
     *
     * The text is read as in parse_time_control(). If that does not work, but the text is already a correct tag value
     * (e.g. "40/7200:3600"), it is taken as it is.
     *
     * @param t_text The text.
     *
     * @return The value for the tag TimeControl, or std::nullopt if the text is not understood.
     */
    [[nodiscard]] std::optional<std::string> time_control_from_input(std::string_view t_text);

    /**
     * @brief Turns the value of the PGN tag into the text shown in an input field.
     *
     * A simple time control is shown in words ("15 min + 10 s"), which time_control_from_input() reads back to the same
     * value. Anything else is shown as it is.
     *
     * @param t_value The value of the tag.
     *
     * @return The text.
     */
    [[nodiscard]] std::string time_control_for_input(std::string_view t_value);
} // namespace sg::chess::core
