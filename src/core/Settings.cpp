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
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string_view>
#include <system_error>
#include <type_traits>
#include "core/Settings.h"
#include "core/Util.h"

namespace
{
    // Json objects are made with "=" and not with braces: braces around a single value make an array.
    using Json = nlohmann::ordered_json;

    // The version of the format of the file. It only goes up if a change cannot be read by the older program.
    constexpr int FORMAT_VERSION{ 1 };

    constexpr const char* FILE_NAME{ "settings.json" };
    constexpr int INDENT{ 2 };

    // A language code is short and has letters and an underscore ("de", "pt_BR"). Anything else would be a mistake.
    constexpr std::size_t MAX_LANGUAGE_LENGTH{ 16 };

    bool IsLanguageCode(const std::string& t_text)
    {
        return !t_text.empty() && t_text.size() <= MAX_LANGUAGE_LENGTH
               && std::ranges::all_of(t_text, [](const char t_c)
                                      { return std::isalpha(static_cast<unsigned char>(t_c)) != 0 || t_c == '_'; });
    }

    // Reads a value of a type if the entry is there and has that type. If not, the value stays as it is.
    template <typename T>
    void Read(const Json& t_object, const char* t_key, T& t_value)
    {
        if (const auto it{ t_object.find(t_key) }; it != t_object.end())
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                if (it->is_boolean())
                {
                    t_value = it->get<bool>();
                }
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (it->is_string())
                {
                    t_value = it->get<std::string>();
                }
            }
        }
    }

    // Puts the entries of t_extra into t_out if t_out does not have them. Objects are merged one level down.
    void Merge(Json& t_out, const Json& t_extra)
    {
        for (const auto& [key, value] : t_extra.items())
        {
            if (!t_out.contains(key))
            {
                t_out[key] = value;
            }
            else if (t_out[key].is_object() && value.is_object())
            {
                Merge(t_out[key], value);
            }
        }
    }

    // The entries that this version knows. An object stands for a group of entries, null for a single entry.
    Json KnownEntries()
    {
        Json known = Json::object();
        known["version"] = nullptr;
        known["language"] = nullptr;
        known["analysis"]["engineOnAtStart"] = nullptr;
        known["accounts"]["chesscom"]["username"] = nullptr;

        return known;
    }

    // The entries of a group that t_known does not have, as an object (empty if there are none). In a group that
    // this version knows, only the entries that it does not know are taken. A known entry with a wrong type is not
    // kept: it is replaced by its default.
    Json UnknownEntries(const Json& t_document, const Json& t_known)
    {
        Json extra = Json::object();

        for (const auto& [key, value] : t_document.items())
        {
            const auto known{ t_known.find(key) };

            if (known == t_known.end())
            {
                extra[key] = value;
            }
            else if (known->is_object() && value.is_object())
            {
                if (const Json inner = UnknownEntries(value, *known); !inner.empty())
                {
                    extra[key] = inner;
                }
            }
        }

        return extra;
    }

    // The name of an account: not longer than MAX_USERNAME_LENGTH and without control characters. White space at the
    // start and at the end is not part of it.
    std::string CleanUsername(const std::string& t_text)
    {
        std::size_t first{ 0 };
        std::size_t last{ t_text.size() };

        while (first < last && std::isspace(static_cast<unsigned char>(t_text[first])) != 0)
        {
            ++first;
        }

        while (last > first && std::isspace(static_cast<unsigned char>(t_text[last - 1])) != 0)
        {
            --last;
        }

        const std::string name{ t_text.substr(first, last - first) };
        const bool hasControl{ std::ranges::any_of(
            name, [](const char t_c) { return std::iscntrl(static_cast<unsigned char>(t_c)) != 0; }) };

        return hasControl || name.size() > sg::chess::core::Settings::MAX_USERNAME_LENGTH ? std::string{} : name;
    }
} // namespace

//-------------------------------------------------
// File
//-------------------------------------------------

std::filesystem::path sg::chess::core::settings_path()
{
    return data_dir() / FILE_NAME;
}

sg::chess::core::LoadedSettings sg::chess::core::load_settings(const std::filesystem::path& t_path)
{
    std::error_code ec;

    if (!std::filesystem::exists(t_path, ec))
    {
        return {};
    }

    std::string text;

    try
    {
        std::ifstream file{ t_path, std::ios::binary };
        if (!std::filesystem::is_regular_file(t_path, ec) || !file)
        {
            return { {}, "The settings file cannot be read: " + t_path.string() };
        }

        text.assign(std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{});
    }
    catch (const std::ios_base::failure&)
    {
        return { {}, "The settings file cannot be read: " + t_path.string() };
    }

    if (auto settings{ settings_from_json(text) })
    {
        return { std::move(*settings), "" };
    }

    // Not JSON: keep the file for the user to look at, and start with the defaults.
    std::filesystem::path backup{ t_path };
    backup += ".bak";
    std::filesystem::rename(t_path, backup, ec);

    return { {}, "The settings file is not valid JSON. It was renamed to " + backup.filename().string() };
}

std::optional<std::string> sg::chess::core::save_settings(const std::filesystem::path& t_path,
                                                          const Settings& t_settings)
{
    std::error_code ec;

    if (t_path.has_parent_path())
    {
        std::filesystem::create_directories(t_path.parent_path(), ec);
    }

    std::filesystem::path temporary{ t_path };
    temporary += ".tmp";

    {
        std::ofstream file{ temporary, std::ios::binary | std::ios::trunc };
        file << settings_to_json(t_settings);
        file.flush();

        if (!file)
        {
            std::filesystem::remove(temporary, ec);
            return "The settings cannot be written: " + temporary.string();
        }
    }

    std::filesystem::rename(temporary, t_path, ec);
    if (ec)
    {
        std::filesystem::remove(temporary, ec);
        return "The settings file cannot be replaced: " + t_path.string();
    }

    return std::nullopt;
}

//-------------------------------------------------
// Text
//-------------------------------------------------

std::optional<sg::chess::core::Settings> sg::chess::core::settings_from_json(const std::string& t_text)
{
    const Json document = Json::parse(t_text, nullptr, false);

    if (!document.is_object())
    {
        return std::nullopt;
    }

    Settings settings;

    std::string language;
    Read(document, "language", language);

    if (IsLanguageCode(language))
    {
        settings.language = language;
    }

    if (const auto it{ document.find("analysis") }; it != document.end() && it->is_object())
    {
        Read(*it, "engineOnAtStart", settings.analysis.engineOnAtStart);
    }

    if (const auto accounts{ document.find("accounts") }; accounts != document.end() && accounts->is_object())
    {
        if (const auto chesscom{ accounts->find("chesscom") }; chesscom != accounts->end() && chesscom->is_object())
        {
            std::string username;
            Read(*chesscom, "username", username);
            settings.accounts.chesscom.username = CleanUsername(username);
        }
    }

    if (const Json extra = UnknownEntries(document, KnownEntries()); !extra.empty())
    {
        // With sorted keys, the same entries are always the same text, whatever the order in the file was.
        settings.extra = nlohmann::json::parse(extra.dump()).dump();
    }

    return settings;
}

std::string sg::chess::core::settings_to_json(const Settings& t_settings)
{
    Json document = Json::object();

    document["version"] = FORMAT_VERSION;
    document["language"] = IsLanguageCode(t_settings.language) ? t_settings.language : Settings::AUTO_LANGUAGE;
    document["analysis"]["engineOnAtStart"] = t_settings.analysis.engineOnAtStart;
    document["accounts"]["chesscom"]["username"] = CleanUsername(t_settings.accounts.chesscom.username);

    if (!t_settings.extra.empty())
    {
        if (const Json extra = Json::parse(t_settings.extra, nullptr, false); extra.is_object())
        {
            Merge(document, extra);
        }
    }

    return document.dump(INDENT) + "\n";
}
