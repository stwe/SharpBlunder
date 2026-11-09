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
#include <ranges>
#include "core/Util.h"

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
    const std::filesystem::path baseDir{ home ? home : "." };
    std::filesystem::path dir{ baseDir / ".chess" };

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    return dir;
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
