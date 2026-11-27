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

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sg::chess::core
{
    //-------------------------------------------------
    // Constants
    //-------------------------------------------------

    /**
     * @brief Separator between moves in a serialized move sequence (e.g., "e2e4 e7e5 g1f3").
     *
     * Used by join_moves() and for parsing with split_str(). The same format is used in the repertoire files,
     * the evaluation cache, the spaced-repetition keys and the UCI "position ... moves" command.
     */
    inline constexpr char MOVE_SEPARATOR{ ' ' };

    //-------------------------------------------------
    // Utils
    //-------------------------------------------------

    /**
     * @brief Finds out which Stockfish executable to start.
     *
     * Uses the first command line argument if there is one, else the environment variable STOCKFISH,
     * else the name "stockfish", which is looked up in the PATH. The Arch/CachyOS package installs it to /usr/bin,
     * which is in the PATH. The Debian/Ubuntu package uses /usr/games, which is not in the PATH of every user.
     * That is why the Engine class tries that directory as well.
     *
     * @param t_argc The number of command line arguments, as passed to main().
     * @param t_argv The command line arguments, as passed to main().
     *
     * @return The name or the path of the Stockfish executable.
     */
    std::string stockfish_path(int t_argc, char** t_argv);

    /**
     * @brief Retrieves and ensures the existence of the application's data directory.
     *
     * The directory is named 'SharpBlunder' and lies in the user's home folder (or in the working directory if HOME is
     * unset). It is not hidden: it holds the games of the user, too, and not only settings. See data_dir_in().
     *
     * @return std::filesystem::path Path to the application data directory.
     */
    std::filesystem::path data_dir();

    /**
     * @brief Retrieves and ensures the existence of the data directory 'SharpBlunder' in a given folder.
     *
     * Only this directory is used. Other directories in the home folder are never read, moved or changed.
     *
     * @param t_home The folder in which the data directory lies, usually the home folder of the user.
     *
     * @return std::filesystem::path Path to the data directory.
     */
    std::filesystem::path data_dir_in(const std::filesystem::path& t_home);

    /**
     * @brief Writes the current local time as text.
     *
     * @param t_format A format as for std::strftime, e.g. "%Y.%m.%d" for the date of a PGN game.
     *
     * @return The formatted time. It is empty if the format gives no text.
     */
    std::string format_local_time(const std::string& t_format);

    /**
     * @brief Splits a string view by a given delimiter.
     *
     * Empty tokens between and after delimiters are preserved ("a\t\t" yields three tokens).
     * An empty input yields no tokens.
     *
     * @param t_s The target string view to split.
     * @param t_d The character delimiter.
     *
     * @return std::vector<std::string> A vector of tokens extracted from the string.
     */
    std::vector<std::string> split_str(std::string_view t_s, char t_d);

    /**
     * @brief Joins a sequence of moves into a single string separated by MOVE_SEPARATOR.
     *
     * The result identifies a position by the moves leading to it, e.g. {"e2e4", "e7e5"} yields "e2e4 e7e5".
     * An empty sequence (the starting position) yields an empty string.
     * To parse the result again, use split_str(result, MOVE_SEPARATOR).
     *
     * @param t_moves The moves in order (UCI notation).
     *
     * @return std::string The serialized move sequence.
     */
    std::string join_moves(const std::vector<std::string>& t_moves);
}
