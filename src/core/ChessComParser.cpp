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
#include <nlohmann/json.hpp>
#include <utility>
#include "core/ChessComParser.h"

namespace
{
    // Json objects are made with "=" and not with braces: braces around a single value make an array.
    using Json = nlohmann::json;

    // A user name of chess.com has up to 25 characters. Some room is left, so that a change there does no harm.
    constexpr std::size_t MAX_NAME_LENGTH{ 50 };

    // Where the games come from.
    constexpr const char* SOURCE{ "chess.com" };

    // The value of a key if it is a string, or an empty text.
    std::string Text(const Json& t_object, const char* t_key)
    {
        const auto it{ t_object.find(t_key) };

        return it != t_object.end() && it->is_string() ? it->get<std::string>() : std::string{};
    }

    // The value of a key if it is a number, or the fallback.
    template <typename T>
    T Number(const Json& t_object, const char* t_key, const T t_fallback)
    {
        const auto it{ t_object.find(t_key) };

        return it != t_object.end() && it->is_number() ? it->get<T>() : t_fallback;
    }

    std::string Lower(std::string t_text)
    {
        std::ranges::transform(t_text, t_text.begin(),
                               [](const unsigned char t_c) { return static_cast<char>(std::tolower(t_c)); });

        return t_text;
    }

    // The value of a tag in a PGN text, e.g. Tag(pgn, "ECO") for [ECO "B90"]. Empty if there is no such tag.
    std::string Tag(const std::string& t_pgn, const std::string& t_name)
    {
        const std::string start{ "[" + t_name + " \"" };
        std::size_t position{ 0 };

        while ((position = t_pgn.find(start, position)) != std::string::npos)
        {
            if (position == 0 || t_pgn[position - 1] == '\n')
            {
                const std::size_t from{ position + start.size() };
                const std::size_t to{ t_pgn.find('"', from) };

                return to == std::string::npos ? std::string{} : t_pgn.substr(from, to - from);
            }

            ++position;
        }

        return {};
    }

    // The accuracy of one side, if the answer has it.
    std::optional<double> Accuracy(const Json& t_game, const char* t_side)
    {
        const auto it{ t_game.find("accuracies") };

        if (it == t_game.end() || !it->is_object())
        {
            return std::nullopt;
        }

        const auto value{ it->find(t_side) };

        if (value == it->end() || !value->is_number())
        {
            return std::nullopt;
        }

        return value->get<double>();
    }

    // The player of one side: name, rating and result code. False if the data is not usable.
    bool ReadSide(const Json& t_game, const char* t_side, std::string& t_name, int& t_rating, std::string& t_result)
    {
        const auto it{ t_game.find(t_side) };

        if (it == t_game.end() || !it->is_object())
        {
            return false;
        }

        t_name = Text(*it, "username");
        t_rating = Number<int>(*it, "rating", 0);
        t_result = Text(*it, "result");

        return !t_name.empty();
    }
} // namespace

//-------------------------------------------------
// Names
//-------------------------------------------------

bool sg::chess::core::is_valid_chesscom_username(const std::string& t_name)
{
    return !t_name.empty() && t_name.size() <= MAX_NAME_LENGTH
           && std::ranges::all_of(t_name, [](const unsigned char t_c)
                                  { return (std::isalnum(t_c) != 0 && t_c < 128) || t_c == '_' || t_c == '-'; });
}

//-------------------------------------------------
// Parsing
//-------------------------------------------------

std::optional<std::vector<std::string>> sg::chess::core::parse_chesscom_archives(const std::string& t_json)
{
    const Json document = Json::parse(t_json, nullptr, false);

    if (!document.is_object() || !document.contains("archives") || !document["archives"].is_array())
    {
        return std::nullopt;
    }

    std::vector<std::string> archives;

    for (const Json& entry : document["archives"])
    {
        if (entry.is_string())
        {
            archives.push_back(entry.get<std::string>());
        }
    }

    return archives;
}

std::optional<sg::chess::core::ParsedMonth> sg::chess::core::parse_chesscom_month(const std::string& t_json,
                                                                                    const std::string& t_user)
{
    const Json document = Json::parse(t_json, nullptr, false);

    if (!document.is_object() || !document.contains("games") || !document["games"].is_array())
    {
        return std::nullopt;
    }

    ParsedMonth month;
    const std::string user{ Lower(t_user) };

    for (const Json& game : document["games"])
    {
        if (!game.is_object())
        {
            ++month.skippedBroken;
            continue;
        }

        // Variants: the board or the rules are different, so the program cannot show them.
        if (const std::string rules{ Text(game, "rules") }; !rules.empty() && rules != "chess")
        {
            ++month.skippedVariants;
            continue;
        }

        GameSummary summary;
        std::string whiteCode;
        std::string blackCode;

        if (!ReadSide(game, "white", summary.white, summary.whiteRating, whiteCode)
            || !ReadSide(game, "black", summary.black, summary.blackRating, blackCode))
        {
            ++month.skippedBroken;
            continue;
        }

        summary.pgn = Text(game, "pgn");
        summary.url = Text(game, "url");
        summary.id = Text(game, "uuid");

        // Without moves there is nothing to analyze. Without an id, the game cannot be told from another.
        if (summary.pgn.empty() || (summary.id.empty() && summary.url.empty()))
        {
            ++month.skippedBroken;
            continue;
        }

        if (summary.id.empty())
        {
            summary.id = summary.url;
        }

        summary.source = SOURCE;
        summary.result = pgn_result(whiteCode, blackCode);
        summary.endReason = classify_result(whiteCode) == Outcome::WIN ? blackCode : whiteCode;

        if (Lower(summary.white) == user)
        {
            summary.userColor = Color::WHITE;
        }
        else if (Lower(summary.black) == user)
        {
            summary.userColor = Color::BLACK;
        }

        if (summary.userColor)
        {
            summary.outcome = outcome_for(summary.result, *summary.userColor);
        }

        summary.timeControl = Text(game, "time_control");
        summary.timeClass = Text(game, "time_class");
        summary.endTime = Number<std::int64_t>(game, "end_time", 0);
        summary.whiteAccuracy = Accuracy(game, "white");
        summary.blackAccuracy = Accuracy(game, "black");
        summary.rated = game.contains("rated") && game["rated"].is_boolean() && game["rated"].get<bool>();
        summary.eco = Tag(summary.pgn, "ECO");
        summary.opening = opening_from_url(Text(game, "eco"));
        summary.moves = count_moves(summary.pgn);

        month.games.push_back(std::move(summary));
    }

    return month;
}
