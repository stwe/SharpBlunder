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

#include <cstdlib>
#include <ctime>
#include <ranges>
#include "core/Util.h"

namespace
{
    // The folder for the games and the settings of the user. It is not hidden, because the user works with the files.
    constexpr const char* DATA_DIR_NAME{ "SharpBlunder" };

    // The hidden folder that earlier versions used.
    constexpr const char* OLD_DATA_DIR_NAME{ ".chess" };
} // namespace

//-------------------------------------------------
// Utils
//-------------------------------------------------

std::string sg::chess::core::stockfish_path(const int t_argc, char** t_argv)
{
    if (t_argc > 1 && *t_argv[1] != '\0')
    {
        return t_argv[1];
    }

    if (const char* env = std::getenv("STOCKFISH"); env && *env != '\0')
    {
        return env;
    }

    return "stockfish";
}

std::filesystem::path sg::chess::core::data_dir()
{
    const char* home{ std::getenv("HOME") };

    return data_dir_in(home ? home : ".");
}

std::filesystem::path sg::chess::core::data_dir_in(const std::filesystem::path& t_home)
{
    const std::filesystem::path dir{ t_home / DATA_DIR_NAME };
    const std::filesystem::path oldDir{ t_home / OLD_DATA_DIR_NAME };

    std::error_code ec;

    // A failed rename is not an error: the old files stay where they are, and a new directory is made.
    if (!std::filesystem::exists(dir, ec) && std::filesystem::is_directory(oldDir, ec))
    {
        std::filesystem::rename(oldDir, dir, ec);
    }

    std::filesystem::create_directories(dir, ec);

    return dir;
}

std::string sg::chess::core::format_local_time(const std::string& t_format)
{
    const std::time_t now{ std::time(nullptr) };

    std::tm local{};
    localtime_r(&now, &local);

    char text[128];
    const std::size_t length{ std::strftime(text, sizeof text, t_format.c_str(), &local) };

    return std::string{ text, length };
}

std::vector<std::string> sg::chess::core::split_str(std::string_view t_s, char t_d)
{
    return t_s
        | std::views::split(t_d)
        | std::views::transform([](auto&& t_part) {
              return std::string(t_part.begin(), t_part.end());
          })
        | std::ranges::to<std::vector<std::string>>();
}

std::string sg::chess::core::join_moves(const std::vector<std::string>& t_moves)
{
    return t_moves
        | std::views::join_with(MOVE_SEPARATOR)
        | std::ranges::to<std::string>();
}
