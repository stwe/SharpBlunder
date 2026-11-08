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
