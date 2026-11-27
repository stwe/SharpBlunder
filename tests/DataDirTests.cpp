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
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "Check.h"
#include "core/Util.h"

namespace
{
    namespace fs = std::filesystem;

    // A new empty folder that takes the place of a home folder.
    fs::path MakeHome(const std::string& t_name)
    {
        const fs::path home{ fs::temp_directory_path() / ("sharpblunder-test-" + t_name) };

        fs::remove_all(home);
        fs::create_directories(home);

        return home;
    }

    void MakeFile(const fs::path& t_path, const std::string& t_text)
    {
        fs::create_directories(t_path.parent_path());
        std::ofstream{ t_path } << t_text;
    }

    std::string ReadFile(const fs::path& t_path)
    {
        std::string text;
        std::ifstream{ t_path } >> text;

        return text;
    }

    void TestNewHome()
    {
        const fs::path home{ MakeHome("new") };

        const fs::path dir{ sg::chess::core::data_dir_in(home) };

        // The folder is named SharpBlunder, it is made, and it is not hidden.
        CHECK(dir == home / "SharpBlunder");
        CHECK(fs::is_directory(dir));
        CHECK(dir.filename().string().front() != '.');
        CHECK(!fs::exists(home / ".chess"));

        // A second call finds the same folder and keeps what is in it.
        MakeFile(dir / "games" / "a.pgn", "first");
        CHECK(sg::chess::core::data_dir_in(home) == dir);
        CHECK(ReadFile(dir / "games" / "a.pgn") == "first");

        fs::remove_all(home);
    }

    void TestOtherFoldersAreLeftAlone()
    {
        const fs::path home{ MakeHome("other") };
        MakeFile(home / ".chess" / "games" / "other.pgn", "other");

        const fs::path dir{ sg::chess::core::data_dir_in(home) };

        // A folder of another program with the name of an earlier version is not touched: nothing is moved or read.
        CHECK(dir == home / "SharpBlunder");
        CHECK(fs::is_directory(dir));
        CHECK(fs::is_empty(dir));
        CHECK(ReadFile(home / ".chess" / "games" / "other.pgn") == "other");

        fs::remove_all(home);
    }
} // namespace

int main()
{
    TestNewHome();
    TestOtherFoldersAreLeftAlone();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
