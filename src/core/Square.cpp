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
