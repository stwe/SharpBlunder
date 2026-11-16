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

#include <iostream>

namespace sg::chess::tests
{
    /**
     * @brief Counts the failed checks of the running test program.
     *
     * @return A reference to the counter.
     */
    inline int& failures()
    {
        static int count{ 0 };
        return count;
    }

    /**
     * @brief Reports a failed check and counts it. The test goes on, so one run shows all failures.
     *
     * @param t_condition The result of the check.
     * @param t_text The checked expression as text.
     * @param t_line The line of the check.
     */
    inline void check(const bool t_condition, const char* t_text, const int t_line)
    {
        if (!t_condition)
        {
            std::cerr << "FAILED (line " << t_line << "): " << t_text << '\n';
            ++failures();
        }
    }
}

#define CHECK(t_condition) sg::chess::tests::check((t_condition), #t_condition, __LINE__)
