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

#include <charconv>
#include <string>
#include "core/Util.h"
#include "engine/UciInfo.h"

namespace
{
    // Reads an integer token. Returns false if the token is not a number.
    bool ToInt(const std::string& t_token, int& t_value)
    {
        const char* end{ t_token.data() + t_token.size() };
        const auto result{ std::from_chars(t_token.data(), end, t_value) };

        return result.ec == std::errc{} && result.ptr == end;
    }
} // namespace

//-------------------------------------------------
// Parser
//-------------------------------------------------

std::optional<sg::chess::engine::AnalysisLine> sg::chess::engine::parse_info_line(const std::string_view t_line)
{
    const std::vector<std::string> tokens{ core::split_str(t_line, ' ') };

    if (tokens.size() < 2 || tokens[0] != "info" || tokens[1] == "string")
    {
        return std::nullopt;
    }

    AnalysisLine line;
    bool hasDepth{ false };
    bool hasScore{ false };

    for (std::size_t i{ 1 }; i < tokens.size(); ++i)
    {
        const std::string& token{ tokens[i] };

        if (token == "depth" && i + 1 < tokens.size())
        {
            hasDepth = ToInt(tokens[++i], line.depth);
        }
        else if (token == "multipv" && i + 1 < tokens.size())
        {
            if (!ToInt(tokens[++i], line.multipv) || line.multipv < 1)
            {
                return std::nullopt;
            }
        }
        else if (token == "score" && i + 2 < tokens.size())
        {
            const std::string& kind{ tokens[++i] };

            if (kind == "cp")
            {
                line.score.kind = core::Score::Kind::CENTIPAWNS;
            }
            else if (kind == "mate")
            {
                line.score.kind = core::Score::Kind::MATE;
            }
            else
            {
                return std::nullopt;
            }

            hasScore = ToInt(tokens[++i], line.score.value);
        }
        else if (token == "lowerbound" || token == "upperbound")
        {
            return std::nullopt;
        }
        else if (token == "pv")
        {
            // The variation is the rest of the line.
            for (++i; i < tokens.size(); ++i)
            {
                const auto move{ core::Move::FromUci(tokens[i]) };
                if (!move)
                {
                    return std::nullopt;
                }

                line.pv.push_back(*move);
            }
        }
    }

    if (!hasDepth || !hasScore || line.pv.empty())
    {
        return std::nullopt;
    }

    return line;
}
