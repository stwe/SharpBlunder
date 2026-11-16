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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "core/Score.h"

namespace
{
    // Same curve as lichess uses for its evaluation bar: 1 / (1 + e^(-k * cp)).
    constexpr double CURVE_FACTOR{ 0.00368208 };

    // A mate in n moves is worth this value minus n. Far above any real advantage in centipawns.
    constexpr int MATE_CENTIPAWNS{ 100000 };
} // namespace

//-------------------------------------------------
// Score
//-------------------------------------------------

sg::chess::core::Score sg::chess::core::Score::FromSideToMove(const bool t_whiteToMove) const noexcept
{
    return t_whiteToMove ? *this : Score{ kind, -value };
}

double sg::chess::core::Score::WhiteShare() const noexcept
{
    if (kind == Kind::MATE)
    {
        // A mate is never "in 0" here: a position that is already mated has no line, see parse_info_line().
        return value >= 0 ? 1.0 : 0.0;
    }

    return 1.0 / (1.0 + std::exp(-CURVE_FACTOR * static_cast<double>(value)));
}

int sg::chess::core::Score::Centipawns() const noexcept
{
    if (kind == Kind::CENTIPAWNS)
    {
        return value;
    }

    const int size{ MATE_CENTIPAWNS - std::abs(value) };

    return value < 0 ? -size : size;
}

std::string sg::chess::core::Score::ToString() const
{
    if (kind == Kind::MATE)
    {
        return std::string(value < 0 ? "-M" : "M") + std::to_string(std::abs(value));
    }

    char text[32];
    std::snprintf(text, sizeof text, "%+.2f", static_cast<double>(value) / 100.0);

    return text;
}
