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

#include <optional>
#include <string_view>
#include <vector>
#include "core/Move.h"
#include "core/Score.h"

namespace sg::chess::engine
{
    //-------------------------------------------------
    // AnalysisLine
    //-------------------------------------------------

    /**
     * @brief One line of an analysis: a variation with its score and the depth it was found at.
     */
    struct AnalysisLine
    {
        int multipv{ 1 }; ///< Rank of the line: 1 is the best one
        int depth{ 0 }; ///< Search depth in plies
        core::Score score; ///< The score, as sent by the engine: from the side to move
        std::vector<core::Move> pv; ///< The variation, starting with the move to play now
    };

    //-------------------------------------------------
    // Parser
    //-------------------------------------------------

    /**
     * @brief Reads a UCI "info" line, e.g. "info depth 12 multipv 2 score cp 31 nodes 9000 pv e2e4 e7e5".
     *
     * Lines without depth, score or variation are no analysis lines and are skipped. That includes
     * "info string ...", "info currmove ..." and the line of a position that is already mated.
     * A score with a bound ("lowerbound" or "upperbound") is skipped as well: it is not final yet.
     *
     * @param t_line A line from the engine.
     *
     * @return The line, or std::nullopt if it is not an analysis line.
     */
    [[nodiscard]] std::optional<AnalysisLine> parse_info_line(std::string_view t_line);
} // namespace sg::chess::engine
