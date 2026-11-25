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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <system_error>
#include "core/I18n.h"

namespace
{
    // In a catalog the context and the text are one key. This is the separator of gettext, too.
    constexpr char CONTEXT_SEPARATOR{ '\x04' };

    // The header of a .po file is the entry with the empty text. This line of it has the name of the language.
    constexpr std::string_view NATIVE_NAME_HEADER{ "X-Native-Name:" };

    // A language code is short and has letters and an underscore ("de", "pt_BR"). It is part of a file name, so
    // nothing else is allowed.
    constexpr std::size_t MAX_CODE_LENGTH{ 16 };

    constexpr std::string_view UTF8_BOM{ "\xEF\xBB\xBF" };

    bool IsCode(const std::string_view t_text)
    {
        return !t_text.empty() && t_text.size() <= MAX_CODE_LENGTH
               && std::ranges::all_of(t_text, [](const char t_c)
                                      { return std::isalpha(static_cast<unsigned char>(t_c)) != 0 || t_c == '_'; });
    }

    std::string MakeKey(const std::string_view t_text, const std::string_view t_context)
    {
        if (t_context.empty())
        {
            return std::string{ t_text };
        }

        std::string key{ t_context };
        key += CONTEXT_SEPARATOR;
        key += t_text;

        return key;
    }

    std::string_view TrimStart(std::string_view t_text)
    {
        while (!t_text.empty() && (t_text.front() == ' ' || t_text.front() == '\t'))
        {
            t_text.remove_prefix(1);
        }

        return t_text;
    }

    std::string_view TrimEnd(std::string_view t_text)
    {
        while (!t_text.empty()
               && (t_text.back() == ' ' || t_text.back() == '\t' || t_text.back() == '\r' || t_text.back() == '\n'))
        {
            t_text.remove_suffix(1);
        }

        return t_text;
    }

    // Reads a quoted text of a .po file: "Hello\n" with the escape sequences. Returns nullopt if it is not quoted.
    std::optional<std::string> Unquote(std::string_view t_text)
    {
        t_text = TrimEnd(TrimStart(t_text));

        if (t_text.size() < 2 || t_text.front() != '"' || t_text.back() != '"')
        {
            return std::nullopt;
        }

        t_text = t_text.substr(1, t_text.size() - 2);

        std::string text;
        for (std::size_t i{ 0 }; i < t_text.size(); ++i)
        {
            if (t_text[i] != '\\' || i + 1 == t_text.size())
            {
                text += t_text[i];
                continue;
            }

            ++i;
            switch (t_text[i])
            {
            case 'n':
                text += '\n';
                break;
            case 't':
                text += '\t';
                break;
            case 'r':
                text += '\r';
                break;
            default:
                // \" and \\ and anything else: the character itself.
                text += t_text[i];
                break;
            }
        }

        return text;
    }

    // Which part of an entry the quoted lines go to.
    enum class Field
    {
        NONE,
        CONTEXT,
        ID,
        PLURAL,
        STRING
    };

    struct Entry
    {
        std::string context;
        std::string id;
        std::string string;
        bool hasId{ false };
        bool hasString{ false };
        bool plural{ false };
        bool fuzzy{ false };
    };

    // The value of a header line "Name: value" in the text of a header, or empty if there is none.
    std::string HeaderValue(const std::string_view t_header, const std::string_view t_name)
    {
        std::size_t start{ 0 };

        while (start < t_header.size())
        {
            std::size_t end{ t_header.find('\n', start) };
            if (end == std::string_view::npos)
            {
                end = t_header.size();
            }

            const std::string_view line{ t_header.substr(start, end - start) };
            if (line.starts_with(t_name))
            {
                return std::string{ TrimEnd(TrimStart(line.substr(t_name.size()))) };
            }

            start = end + 1;
        }

        return "";
    }

    std::map<std::string, sg::chess::core::Catalog>& LoadedCatalogs()
    {
        static std::map<std::string, sg::chess::core::Catalog> catalogs;
        return catalogs;
    }

    const sg::chess::core::Catalog*& ActiveCatalog()
    {
        static const sg::chess::core::Catalog* catalog{ nullptr };
        return catalog;
    }

    std::string& ActiveCode()
    {
        static std::string code{ sg::chess::core::SOURCE_LANGUAGE };
        return code;
    }

    std::optional<std::string> ReadFile(const std::filesystem::path& t_path)
    {
        try
        {
            std::error_code ec;
            std::ifstream file{ t_path, std::ios::binary };

            if (!std::filesystem::is_regular_file(t_path, ec) || !file)
            {
                return std::nullopt;
            }

            return std::string{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
        }
        catch (const std::ios_base::failure&)
        {
            return std::nullopt;
        }
    }
} // namespace

//-------------------------------------------------
// Catalog
//-------------------------------------------------

sg::chess::core::Catalog sg::chess::core::Catalog::FromPo(std::string_view t_text)
{
    Catalog catalog;

    if (t_text.starts_with(UTF8_BOM))
    {
        t_text.remove_prefix(UTF8_BOM.size());
    }

    Entry entry;
    Field field{ Field::NONE };

    // An entry is finished by an empty line or by the start of the next one.
    const auto flush = [&]()
    {
        if (entry.hasId && entry.hasString && !entry.plural && !entry.fuzzy && !entry.string.empty())
        {
            if (entry.id.empty())
            {
                catalog.m_nativeName = HeaderValue(entry.string, NATIVE_NAME_HEADER);
            }
            else
            {
                catalog.m_entries[MakeKey(entry.id, entry.context)] = entry.string;
            }
        }

        entry = Entry{};
        field = Field::NONE;
    };

    std::size_t start{ 0 };

    while (start <= t_text.size())
    {
        std::size_t end{ t_text.find('\n', start) };
        if (end == std::string_view::npos)
        {
            end = t_text.size();
        }

        const std::string_view line{ TrimEnd(TrimStart(t_text.substr(start, end - start))) };
        start = end + 1;

        if (line.empty())
        {
            flush();
            continue;
        }

        if (line.front() == '#')
        {
            // "#, fuzzy" says that the translation is a guess: gettext does not use it, and neither do we.
            // "#~" lines are entries that no longer exist in the program.
            if (line.starts_with("#,") && line.find("fuzzy") != std::string_view::npos)
            {
                entry.fuzzy = true;
            }

            continue;
        }

        if (line.front() == '"')
        {
            const auto text{ Unquote(line) };
            if (!text)
            {
                continue;
            }

            switch (field)
            {
            case Field::CONTEXT:
                entry.context += *text;
                break;
            case Field::ID:
                entry.id += *text;
                break;
            case Field::STRING:
                entry.string += *text;
                break;
            case Field::PLURAL:
            case Field::NONE:
                break;
            }

            continue;
        }

        // "msgid_plural" and "msgstr[0]" start with the same letters as "msgid" and "msgstr", so they come first.
        if (line.starts_with("msgid_plural"))
        {
            entry.plural = true;
            field = Field::PLURAL;
        }
        else if (line.starts_with("msgstr["))
        {
            entry.plural = true;
            field = Field::PLURAL;
        }
        else if (line.starts_with("msgctxt"))
        {
            if (entry.hasId)
            {
                flush();
            }

            if (const auto text{ Unquote(line.substr(7)) })
            {
                entry.context = *text;
                field = Field::CONTEXT;
            }
        }
        else if (line.starts_with("msgid"))
        {
            if (entry.hasId)
            {
                flush();
            }

            if (const auto text{ Unquote(line.substr(5)) })
            {
                entry.id = *text;
                entry.hasId = true;
                field = Field::ID;
            }
        }
        else if (line.starts_with("msgstr"))
        {
            if (const auto text{ Unquote(line.substr(6)) })
            {
                entry.string = *text;
                entry.hasString = true;
                field = Field::STRING;
            }
        }
    }

    flush();

    return catalog;
}

const std::string* sg::chess::core::Catalog::Find(const std::string_view t_text, const std::string_view t_context) const
{
    const auto it{ t_context.empty() ? m_entries.find(t_text) : m_entries.find(MakeKey(t_text, t_context)) };

    return it == m_entries.end() ? nullptr : &it->second;
}

//-------------------------------------------------
// Languages
//-------------------------------------------------

std::vector<sg::chess::core::Language> sg::chess::core::available_languages(const std::filesystem::path& t_directory)
{
    std::vector<Language> languages;

    std::error_code ec;
    for (std::filesystem::directory_iterator it{ t_directory, ec }; !ec && it != std::filesystem::directory_iterator{};
         it.increment(ec))
    {
        const std::filesystem::path& path{ it->path() };
        const std::string code{ path.stem().string() };

        if (path.extension() != ".po" || !IsCode(code) || code == SOURCE_LANGUAGE)
        {
            continue;
        }

        const auto text{ ReadFile(path) };
        if (!text)
        {
            continue;
        }

        std::string name{ Catalog::FromPo(*text).NativeName() };
        languages.push_back({ code, name.empty() ? code : std::move(name) });
    }

    std::ranges::sort(languages, {}, &Language::code);
    languages.insert(languages.begin(), Language{ SOURCE_LANGUAGE, "English" });

    return languages;
}

std::string sg::chess::core::language_from_locale(std::string_view t_locale)
{
    // "de_DE.UTF-8@euro": the language and the region are what is in front of "." and "@".
    t_locale = t_locale.substr(0, t_locale.find_first_of(".@"));

    if (t_locale == "C" || t_locale == "POSIX" || !IsCode(t_locale))
    {
        return "";
    }

    return std::string{ t_locale };
}

std::string sg::chess::core::system_language()
{
    const auto variable = [](const char* t_name) -> std::string
    {
        const char* value{ std::getenv(t_name) };
        return value ? value : "";
    };

    // As in gettext: the first of these that is set decides if there is a language at all ("C" has none) ...
    std::string locale;
    for (const char* name : { "LC_ALL", "LC_MESSAGES", "LANG" })
    {
        if (locale = variable(name); !locale.empty())
        {
            break;
        }
    }

    const std::string language{ language_from_locale(locale) };
    if (language.empty())
    {
        return "";
    }

    // ... and LANGUAGE, a list like "de:en", may name a better one.
    const std::string preferred{ variable("LANGUAGE") };
    if (const std::string first{ language_from_locale(preferred.substr(0, preferred.find(':'))) }; !first.empty())
    {
        return first;
    }

    return language;
}

std::string sg::chess::core::resolve_language(const std::string_view t_setting, const std::string_view t_system,
                                              const std::vector<Language>& t_available)
{
    const std::string wanted{ t_setting == "auto" ? t_system : t_setting };

    const auto has = [&](const std::string_view t_code)
    { return std::ranges::any_of(t_available, [&](const Language& t_language) { return t_language.code == t_code; }); };

    if (has(wanted))
    {
        return wanted;
    }

    // "pt_BR" is not there, but "pt" is.
    if (const std::string language{ wanted.substr(0, wanted.find('_')) }; !language.empty() && has(language))
    {
        return language;
    }

    return SOURCE_LANGUAGE;
}

//-------------------------------------------------
// Translation
//-------------------------------------------------

std::optional<std::string> sg::chess::core::set_language(const std::string& t_code,
                                                         const std::filesystem::path& t_directory)
{
    // Until it has worked, the texts are English.
    ActiveCatalog() = nullptr;
    ActiveCode() = SOURCE_LANGUAGE;

    if (t_code == SOURCE_LANGUAGE)
    {
        return std::nullopt;
    }

    if (!IsCode(t_code))
    {
        return "Not a language code: " + t_code;
    }

    auto& catalogs{ LoadedCatalogs() };

    auto it{ catalogs.find(t_code) };
    if (it == catalogs.end())
    {
        const std::filesystem::path file{ t_directory / (t_code + ".po") };
        const auto text{ ReadFile(file) };

        if (!text)
        {
            return "The translation cannot be read: " + file.string();
        }

        it = catalogs.emplace(t_code, Catalog::FromPo(*text)).first;
    }

    ActiveCatalog() = &it->second;
    ActiveCode() = t_code;

    return std::nullopt;
}

const std::string& sg::chess::core::current_language()
{
    return ActiveCode();
}

const char* sg::chess::core::tr(const char* t_text)
{
    if (const Catalog* catalog{ ActiveCatalog() })
    {
        if (const std::string* translation{ catalog->Find(t_text) })
        {
            return translation->c_str();
        }
    }

    return t_text;
}

const char* sg::chess::core::tr_c(const char* t_context, const char* t_text)
{
    if (const Catalog* catalog{ ActiveCatalog() })
    {
        if (const std::string* translation{ catalog->Find(t_text, t_context) })
        {
            return translation->c_str();
        }
    }

    return t_text;
}
