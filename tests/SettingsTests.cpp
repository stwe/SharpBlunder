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
#include "core/Settings.h"

namespace
{
    namespace fs = std::filesystem;

    using sg::chess::core::Settings;

    // A new empty folder for the files of a test.
    fs::path MakeFolder(const std::string& t_name)
    {
        const fs::path folder{ fs::temp_directory_path() / ("sharpblunder-settings-" + t_name) };

        fs::remove_all(folder);
        fs::create_directories(folder);

        return folder;
    }

    void WriteFile(const fs::path& t_path, const std::string& t_text)
    {
        std::ofstream{ t_path, std::ios::binary } << t_text;
    }

    std::string ReadFile(const fs::path& t_path)
    {
        std::ifstream file{ t_path, std::ios::binary };

        return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
    }

    void TestDefaults()
    {
        const Settings settings;

        CHECK(settings.language == "auto");
        CHECK(settings.analysis.engineOnAtStart);
        CHECK(settings.extra.empty());

        // An empty object is a file without settings: all defaults.
        const auto parsed{ sg::chess::core::settings_from_json("{}") };
        CHECK(parsed.has_value());
        CHECK(parsed == settings);
    }

    void TestText()
    {
        Settings settings;
        settings.language = "de";
        settings.analysis.engineOnAtStart = false;

        const std::string text{ sg::chess::core::settings_to_json(settings) };

        // The file is for people, too: indented, with the version first.
        CHECK(text == "{\n  \"version\": 1,\n  \"language\": \"de\",\n  \"analysis\": {\n"
                      "    \"engineOnAtStart\": false\n  }\n}\n");
        CHECK(sg::chess::core::settings_from_json(text) == settings);
    }

    void TestWrongValues()
    {
        using sg::chess::core::settings_from_json;

        // Entries with a wrong type or a nonsense value fall back to their defaults, the others are read.
        auto settings{ settings_from_json(R"({"language": 5, "analysis": {"engineOnAtStart": "yes"}})") };
        CHECK(settings.has_value());
        CHECK(settings->language == "auto");
        CHECK(settings->analysis.engineOnAtStart);

        settings = settings_from_json(R"({"language": "de", "analysis": {"engineOnAtStart": false}})");
        CHECK(settings->language == "de");
        CHECK(!settings->analysis.engineOnAtStart);

        CHECK(settings_from_json(R"({"language": "../etc"})")->language == "auto");
        CHECK(settings_from_json(R"({"language": ""})")->language == "auto");
        CHECK(settings_from_json(R"({"language": "pt_BR"})")->language == "pt_BR");
        CHECK(settings_from_json(R"({"analysis": 7})")->analysis.engineOnAtStart);
        CHECK(settings_from_json(R"({"analysis": {"engineOnAtStart": false}, "language": null})")->language == "auto");

        // Settings that are wrong in memory are not written.
        Settings broken;
        broken.language = "no way";
        CHECK(sg::chess::core::settings_from_json(sg::chess::core::settings_to_json(broken))->language == "auto");
    }

    void TestNotJson()
    {
        using sg::chess::core::settings_from_json;

        CHECK(!settings_from_json(""));
        CHECK(!settings_from_json("language = de"));
        CHECK(!settings_from_json("[1, 2]"));
        CHECK(!settings_from_json("\"de\""));
        CHECK(!settings_from_json("{\"language\": "));
    }

    void TestUnknownEntries()
    {
        using sg::chess::core::settings_from_json;
        using sg::chess::core::settings_to_json;

        // A later version has added entries. This version keeps them, in the file, when it saves.
        const auto settings{ settings_from_json(R"({
            "version": 2,
            "language": "de",
            "theme": "dark",
            "analysis": {"engineOnAtStart": false, "maxDepth": 30},
            "engine": {"threads": 4}
        })") };

        CHECK(settings.has_value());
        CHECK(settings->language == "de");
        CHECK(!settings->extra.empty());

        const std::string text{ settings_to_json(*settings) };
        CHECK(text.find("\"theme\": \"dark\"") != std::string::npos);
        CHECK(text.find("\"maxDepth\": 30") != std::string::npos);
        CHECK(text.find("\"threads\": 4") != std::string::npos);

        // The known entries are written once, with the values of the settings and not with the old ones.
        Settings changed{ *settings };
        changed.language = "en";
        const std::string changedText{ settings_to_json(changed) };
        CHECK(changedText.find("\"language\": \"en\"") != std::string::npos);
        CHECK(changedText.find("\"language\": \"de\"") == std::string::npos);
        CHECK(settings_from_json(changedText) == changed);

        // The version of this program is written.
        CHECK(changedText.find("\"version\": 1") != std::string::npos);
    }

    void TestFiles()
    {
        using sg::chess::core::load_settings;
        using sg::chess::core::save_settings;

        const fs::path folder{ MakeFolder("files") };
        const fs::path path{ folder / "sub" / "settings.json" };

        // No file: the defaults, and no problem.
        auto loaded{ load_settings(path) };
        CHECK(loaded.settings == Settings{});
        CHECK(loaded.problem.empty());

        // Saving makes the folder, and leaves no temporary file.
        Settings settings;
        settings.language = "de";
        settings.analysis.engineOnAtStart = false;
        CHECK(!save_settings(path, settings));
        CHECK(fs::exists(path));
        CHECK(!fs::exists(folder / "sub" / "settings.json.tmp"));

        loaded = load_settings(path);
        CHECK(loaded.settings == settings);
        CHECK(loaded.problem.empty());

        // Saving again replaces the file.
        settings.language = "en";
        CHECK(!save_settings(path, settings));
        CHECK(load_settings(path).settings.language == "en");

        fs::remove_all(folder);
    }

    void TestBrokenFile()
    {
        using sg::chess::core::load_settings;

        const fs::path folder{ MakeFolder("broken") };
        const fs::path path{ folder / "settings.json" };

        // A file that is not JSON is kept as .bak, and the program starts with the defaults.
        WriteFile(path, "this is { not json");
        auto loaded{ load_settings(path) };
        CHECK(loaded.settings == Settings{});
        CHECK(!loaded.problem.empty());
        CHECK(!fs::exists(path));
        CHECK(ReadFile(folder / "settings.json.bak") == "this is { not json");

        // The next broken file replaces the old backup. Nothing throws.
        WriteFile(path, "[]");
        loaded = load_settings(path);
        CHECK(!loaded.problem.empty());
        CHECK(ReadFile(folder / "settings.json.bak") == "[]");

        // A folder where the file should be: reading fails, but nothing throws.
        fs::create_directories(folder / "dir.json");
        CHECK(load_settings(folder / "dir.json").settings == Settings{});

        fs::remove_all(folder);
    }

    void TestUnwritable()
    {
        using sg::chess::core::save_settings;

        const fs::path folder{ MakeFolder("unwritable") };

        // The place of the file is a folder: the error is reported, and nothing throws.
        fs::create_directories(folder / "settings.json");
        CHECK(save_settings(folder / "settings.json", Settings{}).has_value());
        CHECK(!fs::exists(folder / "settings.json.tmp"));

        fs::remove_all(folder);
    }
} // namespace

int main()
{
    TestDefaults();
    TestText();
    TestWrongValues();
    TestNotJson();
    TestUnknownEntries();
    TestFiles();
    TestBrokenFile();
    TestUnwritable();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
