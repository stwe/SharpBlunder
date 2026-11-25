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
#include <vector>
#include "Check.h"
#include "core/I18n.h"

namespace
{
    namespace fs = std::filesystem;

    using sg::chess::core::Catalog;
    using sg::chess::core::Language;

    void WriteFile(const fs::path& t_path, const std::string& t_text)
    {
        fs::create_directories(t_path.parent_path());
        std::ofstream{ t_path, std::ios::binary } << t_text;
    }

    const std::string GERMAN{ R"(# German translation
msgid ""
msgstr ""
"Content-Type: text/plain; charset=UTF-8\n"
"X-Native-Name: Deutsch\n"

msgid "Save"
msgstr "Speichern"

#. A comment for the translator
msgid "Cancel"
msgstr "Abbrechen"

msgid "Draw"
msgstr "Remis"

msgctxt "noun"
msgid "Move"
msgstr "Zug"

msgctxt "verb"
msgid "Move"
msgstr "Ziehen"

msgid "{0} mates in {1}"
msgstr "{0} setzt in {1} matt"
)" };

    void TestParse()
    {
        const Catalog catalog{ Catalog::FromPo(GERMAN) };

        CHECK(catalog.Size() == 6);
        CHECK(catalog.NativeName() == "Deutsch");
        CHECK(catalog.Find("Save") != nullptr && *catalog.Find("Save") == "Speichern");
        CHECK(*catalog.Find("Cancel") == "Abbrechen");
        CHECK(*catalog.Find("Draw") == "Remis");

        // A text that is not in the catalog, and the header, are not translations.
        CHECK(catalog.Find("Quit") == nullptr);
        CHECK(catalog.Find("") == nullptr);

        // Equal English texts with different contexts.
        CHECK(*catalog.Find("Move", "noun") == "Zug");
        CHECK(*catalog.Find("Move", "verb") == "Ziehen");
        CHECK(catalog.Find("Move") == nullptr);
        CHECK(catalog.Find("Move", "other") == nullptr);
    }

    void TestParseDetails()
    {
        // Long texts in several lines, and the escape sequences.
        const Catalog catalog{ Catalog::FromPo(R"(msgid ""
"Line one\n"
"Line \"two\""
msgstr ""
"Zeile eins\n"
"Zeile \"zwei\"\t\\"
)") };

        CHECK(catalog.Size() == 1);
        const std::string* text{ catalog.Find("Line one\nLine \"two\"") };
        CHECK(text != nullptr);
        if (text)
        {
            CHECK(*text == "Zeile eins\nZeile \"zwei\"\t\\");
        }
    }

    void TestParseSkips()
    {
        // fuzzy entries (guesses), entries without a translation, plural entries and old entries are not used.
        const Catalog catalog{ Catalog::FromPo(R"(#, fuzzy
msgid "Guess"
msgstr "Raten"

msgid "Empty"
msgstr ""

msgid "One file"
msgid_plural "{} files"
msgstr[0] "Eine Datei"
msgstr[1] "{} Dateien"

#~ msgid "Old"
#~ msgstr "Alt"

msgid "Kept"
msgstr "Behalten"
)") };

        CHECK(catalog.Size() == 1);
        CHECK(catalog.Find("Guess") == nullptr);
        CHECK(catalog.Find("Empty") == nullptr);
        CHECK(catalog.Find("One file") == nullptr);
        CHECK(catalog.Find("Old") == nullptr);
        CHECK(catalog.Find("Kept") != nullptr);
    }

    void TestParseUnusualFiles()
    {
        // A byte order mark, Windows line ends, no empty line at the end, and entries without an empty line between.
        const Catalog catalog{ Catalog::FromPo(
            "\xEF\xBB\xBFmsgid \"A\"\r\nmsgstr \"a\"\r\nmsgid \"B\"\r\nmsgstr \"b\"") };

        CHECK(catalog.Size() == 2);
        CHECK(*catalog.Find("A") == "a");
        CHECK(*catalog.Find("B") == "b");

        // Nothing that is a .po file: an empty catalog, and no exception.
        CHECK(Catalog::FromPo("").Size() == 0);
        CHECK(Catalog::FromPo("this is not a po file\nat all").Size() == 0);
        CHECK(Catalog::FromPo("msgid A\nmsgstr B\n").Size() == 0);
        CHECK(Catalog::FromPo("msgid \"A\"\nmsgstr \"unfinished\n").Size() == 0);
        CHECK(Catalog::FromPo("msgstr \"no id\"\n").Size() == 0);

        // A header without the name of the language.
        CHECK(Catalog::FromPo("msgid \"\"\nmsgstr \"Language: de\\n\"\n").NativeName().empty());
    }

    void TestLocale()
    {
        using sg::chess::core::language_from_locale;

        CHECK(language_from_locale("de_DE.UTF-8") == "de_DE");
        CHECK(language_from_locale("de_DE") == "de_DE");
        CHECK(language_from_locale("de") == "de");
        CHECK(language_from_locale("de_DE@euro") == "de_DE");
        CHECK(language_from_locale("en_US.utf8") == "en_US");
        CHECK(language_from_locale("C") == "");
        CHECK(language_from_locale("C.UTF-8") == "");
        CHECK(language_from_locale("POSIX") == "");
        CHECK(language_from_locale("") == "");
        CHECK(language_from_locale("../../etc") == "");
    }

    void TestResolve()
    {
        using sg::chess::core::resolve_language;

        const std::vector<Language> available{ { "en", "English" }, { "de", "Deutsch" }, { "pt", "Português" } };

        // "auto" follows the system, with the region left out if only the language is there.
        CHECK(resolve_language("auto", "de_DE", available) == "de");
        CHECK(resolve_language("auto", "de", available) == "de");
        CHECK(resolve_language("auto", "pt_BR", available) == "pt");
        CHECK(resolve_language("auto", "en_US", available) == "en");

        // A language that there is no translation for, and a system without a language: English.
        CHECK(resolve_language("auto", "fr_FR", available) == "en");
        CHECK(resolve_language("auto", "", available) == "en");

        // The setting of the user wins over the system.
        CHECK(resolve_language("en", "de_DE", available) == "en");
        CHECK(resolve_language("de", "en_US", available) == "de");
        CHECK(resolve_language("fr", "de_DE", available) == "en");
        CHECK(resolve_language("de", "", {}) == "en");
    }

    void TestFilesAndTr()
    {
        using sg::chess::core::available_languages;
        using sg::chess::core::set_language;
        using sg::chess::core::tr;

        const fs::path folder{ fs::temp_directory_path() / "sharpblunder-i18n-test" };
        fs::remove_all(folder);

        // No folder: only English.
        const std::vector<Language> onlyEnglish{ { "en", "English" } };
        CHECK(available_languages(folder / "nothing") == onlyEnglish);

        WriteFile(folder / "de.po", GERMAN);
        WriteFile(folder / "fr.po", "msgid \"Save\"\nmsgstr \"Enregistrer\"\n");
        WriteFile(folder / "notes.txt", "not a catalog");
        WriteFile(folder / "bad name.po", "msgid \"A\"\nmsgstr \"B\"\n");
        WriteFile(folder / "en.po", "msgid \"Save\"\nmsgstr \"Store\"\n");

        // English first, then by code. Without the native name the code is shown.
        const std::vector<Language> languages{ available_languages(folder) };
        const std::vector<Language> expected{ { "en", "English" }, { "de", "Deutsch" }, { "fr", "fr" } };
        CHECK(languages == expected);

        // Before anything is chosen, the texts are English.
        CHECK(std::string{ tr("Save") } == "Save");
        CHECK(sg::chess::core::current_language() == "en");

        CHECK(!set_language("de", folder));
        CHECK(sg::chess::core::current_language() == "de");
        CHECK(std::string{ tr("Save") } == "Speichern");
        CHECK(std::string{ tr("Draw") } == "Remis");
        CHECK(std::string{ sg::chess::core::tr_c("noun", "Move") } == "Zug");
        CHECK(std::string{ sg::chess::core::tr_c("verb", "Move") } == "Ziehen");

        // A text without a translation stays English, and is the very same pointer.
        const char* untranslated{ "Not in the catalog" };
        CHECK(tr(untranslated) == untranslated);

        // A text that was given out stays valid when the language changes.
        const char* german{ tr("Save") };
        CHECK(!set_language("fr", folder));
        CHECK(std::string{ tr("Save") } == "Enregistrer");
        CHECK(std::string{ german } == "Speichern");

        CHECK(!set_language("en", folder));
        CHECK(std::string{ tr("Save") } == "Save");

        // Back to German: no new file is read (it is gone now), the catalog is still there.
        fs::remove(folder / "de.po");
        CHECK(!set_language("de", folder));
        CHECK(std::string{ tr("Save") } == "Speichern");

        // What cannot be loaded leaves English. A name with a path in it is not even tried.
        CHECK(set_language("it", folder).has_value());
        CHECK(sg::chess::core::current_language() == "en");
        CHECK(std::string{ tr("Save") } == "Save");
        CHECK(set_language("../de", folder).has_value());

        fs::remove_all(folder);
    }

    void TestFormat()
    {
        using sg::chess::core::set_language;
        using sg::chess::core::tr_format;

        const fs::path folder{ fs::temp_directory_path() / "sharpblunder-i18n-format" };
        fs::remove_all(folder);

        WriteFile(folder / "de.po", GERMAN);
        WriteFile(folder / "xx.po", "msgid \"{0} mates in {1}\"\nmsgstr \"{1} {0} {2}\"\n");
        WriteFile(folder / "yy.po", "msgid \"{0} mates in {1}\"\nmsgstr \"in {1} mates {0}\"\n");

        CHECK(!set_language("en", folder));
        CHECK(tr_format("{0} mates in {1}", "White", 3) == "White mates in 3");
        CHECK(tr_format("{} mates in {}", "Black", 12) == "Black mates in 12");
        CHECK(tr_format("No places") == "No places");

        CHECK(!set_language("de", folder));
        CHECK(tr_format("{0} mates in {1}", "Weiß", 3) == "Weiß setzt in 3 matt");

        // A translation may change the order of the places.
        CHECK(!set_language("yy", folder));
        CHECK(tr_format("{0} mates in {1}", "White", 3) == "in 3 mates White");

        // A broken translation (a place that does not exist) falls back to English instead of throwing.
        CHECK(!set_language("xx", folder));
        CHECK(tr_format("{0} mates in {1}", "White", 3) == "White mates in 3");

        CHECK(!set_language("en", folder));
        fs::remove_all(folder);
    }
} // namespace

int main()
{
    TestParse();
    TestParseDetails();
    TestParseSkips();
    TestParseUnusualFiles();
    TestLocale();
    TestResolve();
    TestFilesAndTr();
    TestFormat();

    if (sg::chess::tests::failures() != 0)
    {
        std::cerr << sg::chess::tests::failures() << " check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All checks passed\n";

    return EXIT_SUCCESS;
}
