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

#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include "ui/App.h"
#include "core/Util.h"

int main(const int argc, char** argv)
{
    // Writing to a pipe whose reader has gone must not kill the program (e.g. Stockfish crashed).
    // The write then fails with an error, which the engine class reports as an exception.
    std::signal(SIGPIPE, SIG_IGN);

    try
    {
        sg::chess::ui::App app(sg::chess::core::stockfish_path(argc, argv));
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
