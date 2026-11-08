#include "core/Square.h"

//-------------------------------------------------
// Squares
//-------------------------------------------------

std::string sg::chess::core::square_name(const int t_square)
{
    return std::string{ static_cast<char>('a' + file_of(t_square)), static_cast<char>('1' + rank_of(t_square)) };
}

std::optional<int> sg::chess::core::square_from_name(const std::string_view t_name)
{
    if (t_name.size() != 2 || t_name[0] < 'a' || t_name[0] > 'h' || t_name[1] < '1' || t_name[1] > '8')
    {
        return std::nullopt;
    }

    return square_index(t_name[0] - 'a', t_name[1] - '1');
}
